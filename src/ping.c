#include "ping.h"

/*
ICMP (Internet Control Message Protocol):
    - contrôle, de diagnostic et de rapport d'erreurs
    - struct icmphdr (icmp header) union un dans la struct car on a plusieur interpretation des 4 derniers octects donc dans le union on prend echo c'est l'interpretation de ping:
        - type (uint8_t): Type de message (ICMP_ECHO -> Echo request demande, ICMP_ECHOREPLY -> Echo reply reponse, ICMP_TIME_EXCEEDED -> Erreur TTL trop de saut TTL donc paquet detruit, ICMP_DEST_UNREACH -> destinataire injoignable)
        - code (uint8_t): code d'erreur apporte une precision au type (ICMP_DEST_UNREACH + code 0 reseau entier inaccesible, code 1 l'hote n'existe pas...)
        - checksum (uint16_t): Somme de controle verifie l'integrite du paquet
        - id (uint16_t): Identifiant unique (PID du processus) Pour verifier a quelle processus appartient la reponse on met le pid du programme emetteur
        - sequence (uint16_t): Numero de la sequence
*/

bool sendPacket(struct sockaddr_in *srcAddress,
                int sock, struct arguments *arguments) {
  struct stats stat = {0};
  struct ping_pkt packet = {0};
  stat.minTimeTrip = INFINITY;
  struct timeval startTv, endTv;
  ssize_t lenSend = 0;
  ssize_t lenRecv = 0;
  char buffer[IP_MAXPACKET];
  socklen_t size = sizeof(struct sockaddr_in);

  createPacket(&packet);

  printHeader(arguments);
  while (!stop) {
    packet.hdr.checksum = 0;
    packet.hdr.checksum = checksum(&packet, sizeof(packet));
    lenSend = sendto(sock, &packet, sizeof(packet), 0,
                     (struct sockaddr *)&arguments->destAddress, size); // Cast sockaddr * car c'est la structure generique
    gettimeofday(&startTv, NULL);
    lenRecv = recvfrom(sock, buffer, sizeof(buffer), 0,
                       (struct sockaddr *)srcAddress, &size);
    gettimeofday(&endTv, NULL);
    if (lenSend < 0)
      return (perror("Error: "), false);

    if (lenRecv > 0) { // Vérifie que le paquet a été reçu
      double rtt = (endTv.tv_sec - startTv.tv_sec) * 1000.0 + (endTv.tv_usec - startTv.tv_usec) / 1000.0;
      
      /*
      Recvfrom avec une socketRaw donne le paquet IPV4 complet dans buffer donc ip header + ICMP header + msg
      ip->ihl c'est la longeure de l'en-tete ip en bloc de 4 octects exemple si ca vaut 4 c'est donc 4 bloc de 4 octects
      */
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
      } else {
        // Erreurs ICMP
        char srcIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &srcAddress->sin_addr, srcIp, sizeof(srcIp));
        const char *error_msg = "Unknown ICMP Error";
        
        switch (response->type) {
          case ICMP_DEST_UNREACH: // 3
              switch (response->code) {
                  case 0: error_msg = "Destination network unreachable"; break;
                  case 1: error_msg = "Destination host unreachable"; break;
                  case 2: error_msg = "Destination protocol unreachable"; break;
                  case 3: error_msg = "Destination port unreachable"; break;
                  case 4: error_msg = "Fragmentation required, and DF flag set"; break;
                  case 5: error_msg = "Source route failed"; break;
                  case 6: error_msg = "Destination network unknown"; break;
                  case 7: error_msg = "Destination host unknown"; break;
                  case 8: error_msg = "Source host isolated"; break;
                  case 9: error_msg = "Network administratively prohibited"; break;
                  case 10: error_msg = "Host administratively prohibited"; break;
                  case 11: error_msg = "Network unreachable for ToS"; break;
                  case 12: error_msg = "Host unreachable for ToS"; break;
                  case 13: error_msg = "Communication administratively prohibited"; break;
                  case 14: error_msg = "Host Precedence Violation"; break;
                  case 15: error_msg = "Precedence cutoff in effect"; break;
                  default: error_msg = "Destination Unreachable (unknown code)"; break;
              }
              break;
          case ICMP_SOURCE_QUENCH: // 4
              error_msg = "Source quench (congestion control)"; break;
          case ICMP_REDIRECT: // 5
              switch (response->code) {
                  case 0: error_msg = "Redirect Datagram for the Network"; break;
                  case 1: error_msg = "Redirect Datagram for the Host"; break;
                  case 2: error_msg = "Redirect Datagram for the ToS & network"; break;
                  case 3: error_msg = "Redirect Datagram for the ToS & host"; break;
                  case 6: error_msg = "Alternate Host Address"; break;
                  default: error_msg = "Redirect Message (unknown code)"; break;
              }
              break;
          case ICMP_ECHO: // 8
              error_msg = "Echo request"; break;
          case 9: // Router Advertisement
              error_msg = "Router Advertisement"; break;
          case 10: // Router Solicitation
              error_msg = "Router discovery/selection/solicitation"; break;
          case ICMP_TIME_EXCEEDED: // 11
              switch (response->code) {
                  case 0: error_msg = "Time to live (TTL) expired in transit"; break;
                  case 1: error_msg = "Fragment reassembly time exceeded"; break;
                  default: error_msg = "Time Exceeded (unknown code)"; break;
              }
              break;
          case ICMP_PARAMETERPROB: // 12
              switch (response->code) {
                  case 0: error_msg = "Pointer indicates the error"; break;
                  case 1: error_msg = "Missing a required option"; break;
                  case 2: error_msg = "Bad length"; break;
                  default: error_msg = "Parameter Problem: Bad IP header"; break;
              }
              break;
          default:
              error_msg = "Unknown ICMP Error"; break;
        }
        
        printf("%ld bytes from %s: %s\n", lenRecv - (ip->ihl * 4), srcIp, error_msg);
        if (arguments->verboseIsEnable)
          printRespHeader(buffer);
      }
    }

    stat.packetTransmitted++;

    packet.hdr.un.echo.sequence++;
    if (lenRecv > 0)
        sleep(1);
  }

  printStat(&stat, arguments);

  return (true);
}

bool ping(struct arguments *arguments) {
  struct sockaddr_in srcAddress;
  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); // Creation socket raw pour le protocole IMCP aveec IPV4
  if (sock < 0) {
    fprintf(stderr, "ft_ping: socket: Operation not permitted\n");
    return (false);
  }
  struct timeval timeout;

  int ttl = 1;
  setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

  // Init d'un timeout pour recvfrom si pas de reponse au bout de 1sec
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  // Donne un timeout a la socket pour ne pas rester bloquer sur recvfrom(bloquante)
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  sendPacket(&srcAddress, sock, arguments);

  return (true);
}