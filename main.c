#include "main.h"
#include "printUtils.h"
#include <stdio.h>

int stop = 0;

bool resolveHost(struct arguments *arguments) {
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // Forcer IPv4
  int status = getaddrinfo(arguments->host, NULL, &hints, &res);
  if (status != 0) {
    return (false);
  }
  // Récupérer l'adresse IPv4
  struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
  arguments->destAddress = *ipv4;
  memcpy(arguments->address, &ipv4->sin_addr, sizeof(struct in_addr));
  inet_ntop(AF_INET, &ipv4->sin_addr, arguments->addressPrintable,
            INET_ADDRSTRLEN);
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
  struct sigaction act;
  bzero(&act, sizeof(act));
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
  struct sockaddr_in destAddress;
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
  struct arguments arguments;
  memset(&arguments, 0, sizeof(struct arguments));

  if (parseArg(argc, argv, &arguments) == false)
    return (1);
  if (arguments.helpIsEnable)
    return (printHelp(), 1);
  ping(&arguments);
  return (0);
}