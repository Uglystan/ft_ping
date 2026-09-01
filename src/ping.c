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
  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); // Creation socket raw pour le protocole IMCP aveec IPV4
  if (sock < 0) {
    printf("ft_ping: socket");
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