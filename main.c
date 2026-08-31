#include "main.h"
#include "printUtils.h"
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

/*
struct sockaddr_in exclusivement dedie a l'IPV4:
    - sin_family (2 octects): pour dire qu'elle type d'adresse (AF_INET = IPV4)
    - sin_port (2 octects): Le numero de port a 0 pour ping
    - sin_addr (4 octects): Chaque octect est un nombre de l'adresse ip (sous structure uint32_t s_addr)
    - sin_zero[8] (8 octects): Remplis de 0 pour que la structure fasse 16 octects comme sockaddr qui est la structure generique
*/

/*
struct addrinfo pour communiquer avec getaddrinfo:
    - ai_flags (int): Option supplementaire specifique
    - ai_family (int): Pour specifier un type d'ip de retour (IPV4 = AF_INET, IPV6 AF_INET6 ou les deux AF_UNSPEC)
    - ai_socktype (int): type de socket (SOCK_STREAM -> TCP, SOCK_RAW -> socket brut)
    - ai_protocol (int): Protocole utilise (0 pour ping mais sinon ca sert a preciser si on veut du TCP ou de l'UDP)
    - ai_addrlen (socklen_t): Taille de ai_addr (IPV4 16octects)
    - ai_addr (* sockaddr): Adresse IP resolue -> pointeur vers sockaddr_in
    - ai_canonname (* char): Le nom d'hote officiel
    - ai_next (*addrinfo): Pointeur vers le resultat suivant de la lsite chainee (1 nom de domaine peut avoir plusieurs IP dans le monde)
*/

/*
ICMP (Internet Control Message Protocol):
    - contrôle, de diagnostic et de rapport d'erreurs
    - struct icmphdr (icmp header) union un dans la struct car on a plusieur interpretation des 4 derniers octects donc dans le union on prend echo c'est l'interpretation de ping:
        - type (uint8_t): Type de message (ICMP_ECHO -> Echo request demande, ICMP_ECHOREPLY -> Echo reply reponse, ICMP_TIME_EXCEEDED -> Erreur TTL trop de saut TTL donc paquet detruit, ICMP_DEST_UNREACH -> destinataire injoignable)
        - code (uint8_t): code d'erreur apporte une precision au type (ICMP_DEST_UNREACH + code 0 reseau entier inaccesible, code 1 l'hote n'existe pas...)
        - checksum (uint16_t): Somme de controle verifie l'integrite du paquet
        - id (uint16_t): Identifiant unique (PID du processus) Pour verifier a quelel processus appartient la reponse on met le pid du programme emetteur
        - sequence (uint16_t): Numero de la sequence
*/

int stop = 0;

bool resolveHost(struct arguments *arguments) {
  struct addrinfo hints = {0}; // Restriction de recherche pour getaddrinfo
  struct addrinfo *res = NULL; // 

  hints.ai_family = AF_INET; // Forcer IPv4 pour n'avoir que les IPV4
  int status = getaddrinfo(arguments->host, NULL, &hints, &res); // Resolution DNS pas de port car socket_raw
  if (status != 0) {
    return (false);
  }
  // Récupérer l'adresse IPv4
  struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr; // On a uniquement besoins de l'IP du nom de domaine (sockaddr_in uniquement pour IPV4)
  arguments->destAddress = *ipv4;
  inet_ntop(AF_INET, &ipv4->sin_addr, arguments->addressPrintable, INET_ADDRSTRLEN); // Convertit l'ip en texte
  freeaddrinfo(res);
  return (true);
}

bool parseArg(int argc, char **argv, struct arguments *arguments) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-?") == 0) {
      arguments->helpIsEnable = true;
      return (true);
    } else if (strcmp(argv[i], "-v") == 0) {
      arguments->verboseIsEnable = true;
    } else if (argv[i][0] == '-') {
      printf("ping: invalid option -- '%s'\nTry 'ping --help' or 'ping "
             "--usage' for more information.\n",
             argv[i]);
      return (false);
    } else {
      arguments->host = argv[i];
    }
  }
  if (arguments->host == NULL) {
    printf("ping: missing host operand\nTry 'ping --help' or 'ping --usage' "
           "for more information.\n");
    return (false);
  }
  if (!resolveHost(arguments)) {
    printf("ping: unknown host\n");
    return (false);
  }
  return (true);
}

void sigint_handler(int sig) {
  if (sig == SIGINT)
    stop = 1;
}

void setSignalAction(void) {
  struct sigaction act = {0};
  act.sa_handler = &sigint_handler;
  sigaction(SIGINT, &act, NULL);
}

// Checksum permet de verfifier qu'il n'y a pas eu de corruption du packet
// durant l'envoie on additione tout les elements du packet ce qui donne un
// nombre et ensuite ou l'invers "~sum". Quand i sera recu par le destinataire
// il va add tout les champs du packet dont le checksum (reverse) et il doit
// s'attendre a trouver 0xFFFF
unsigned short checksum(void *temp, int len) {
  unsigned short *buf = temp;
  unsigned int sum = 0;
  unsigned short result;

  for (sum = 0; len > 1; len -= 2)
    sum += *buf++;
  if (len == 1)
    sum += *(unsigned char *)buf;

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  result = ~sum;
  return result;
}

void createPacket(struct ping_pkt *packet) {
  packet->hdr.type = ICMP_ECHO;
  packet->hdr.code = 0;
  packet->hdr.un.echo.id = getpid() & 0xFFFF;
  packet->hdr.un.echo.sequence = 0;

  // Remplir les 56 octets de données avec un motif
  for (size_t i = 0; i < sizeof(packet->msg); i++) {
    packet->msg[i] = (char)i;
  }
}

bool sendPacket(struct sockaddr_in *srcAddress,
                int sock, struct arguments *arguments) {
  struct stats stat = {0};
  struct ping_pkt packet = {0};
  stat.minTimeTrip = INFINITY;
  struct timeval startTv, endTv;
  ssize_t lenSend, lenRecv = 0;
  char buffer[IP_MAXPACKET];
  socklen_t size = sizeof(struct sockaddr_in);

  createPacket(&packet);

  printHeader(arguments);
  while (!stop) {
    packet.hdr.checksum = 0;
    packet.hdr.checksum = checksum(&packet, sizeof(packet));
    lenSend = sendto(sock, &packet, sizeof(packet), 0,
                     (struct sockaddr *)&arguments->destAddress, size);
    gettimeofday(&startTv, NULL);
    lenRecv = recvfrom(sock, buffer, sizeof(buffer), 0,
                       (struct sockaddr *)srcAddress, &size);
    gettimeofday(&endTv, NULL);
    if (lenSend < 0)
      return (perror("Error: "), false);

    if (lenRecv > 0) { // Vérifie que le paquet a été reçu
      double rtt = (endTv.tv_sec - startTv.tv_sec) * 1000.0 +
                   (endTv.tv_usec - startTv.tv_usec) / 1000.0;

      struct iphdr *ip = (struct iphdr *)buffer;
      struct icmphdr *response = (struct icmphdr *)(buffer + (ip->ihl * 4));

      if (response->type == ICMP_ECHOREPLY) {
        // 1. Succès
        if (response->un.echo.id == (getpid() & 0xFFFF)) {
          printData(buffer, rtt, &arguments->destAddress, lenRecv);
          stat.packetReceived++;
          if (rtt > stat.maxTimeTrip)
            stat.maxTimeTrip = rtt;
          if (rtt < stat.minTimeTrip)
            stat.minTimeTrip = rtt;
          stat.totTimeTrip += rtt;
          stat.sqrTimeTrip += rtt * rtt;
        }
      } else if (response->type == ICMP_TIME_EXCEEDED) {
        // 2. Erreur : TTL expiré
        char srcIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &srcAddress->sin_addr, srcIp, sizeof(srcIp));
        printf("%ld bytes from %s: Time to live exceeded\n",
               lenRecv - (ip->ihl * 4), srcIp);
        if (arguments->verboseIsEnable)
          printRespHeader(buffer);
      } else if (response->type == ICMP_DEST_UNREACH) {
        // 3. Erreur : Destination unreachable
        char srcIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &srcAddress->sin_addr, srcIp, sizeof(srcIp));
        printf("%ld bytes from %s: Destination Host Unreachable\n",
               lenRecv - (ip->ihl * 4), srcIp);
        if (arguments->verboseIsEnable)
          printRespHeader(buffer);
      }
    }

    stat.packetTransmitted++;

    packet.hdr.un.echo.sequence++;
    sleep(1);
  }

  printStat(&stat, arguments);

  return (true);
}

bool ping(struct arguments *arguments) {
  struct sockaddr_in srcAddress;
  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sock < 0) {
    printf("ft_ping: socket");
    return (false);
  }
  struct timeval timeout;

//   int ttl = 1;
//   setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

  // Init d'un tiimeout pour recvfrom si pas de reponse au bout de 1sec
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  // Donne un timeout a la socket pour ne pas rester bloquer sur recvfrom
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  sendPacket(&srcAddress, sock, arguments);

  return (true);
}

// resolution DNS (google.com), Si ping marche pas
int main(int argc, char **argv) {
  setSignalAction();
  struct arguments arguments = {0};

  if (parseArg(argc, argv, &arguments) == false)
    return (1);
  if (arguments.helpIsEnable)
    return (printHelp(), 1);
  ping(&arguments);
  return (0);
}