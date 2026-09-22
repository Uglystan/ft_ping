#include "printUtils.h"

void printHelp(void) {
  printf("Usage: ping [OPTION...] HOST ...\nSend ICMP ECHO_REQUEST packets to "
         "network hosts.\n\nOptions valid for all request types:\n\n -v        "
         "      verbose output\n\nMandatory or optional arguments to long "
         "options are also mandatory or optional\nfor any corresponding short "
         "options.\n\nOptions marked with (root only) are available only to "
         "superuser.\n");
}

void printHeader(struct arguments *arguments) {
  uint16_t id = getpid() & 0xFFFF;

  if (arguments->verboseIsEnable) {
    printf("PING %s (%s): 56 data bytes, id 0x%04x = %d\n", arguments->host,
           arguments->addressPrintable, id, id);
  } else {
    printf("PING %s (%s): 56 data bytes\n", arguments->host,
           arguments->addressPrintable);
  }
}

void printData(char *buffer, double rtt, struct sockaddr_in *destAddress, ssize_t lenRecv) {
  struct iphdr *ip = (struct iphdr *)buffer;
  struct icmphdr *response = (struct icmphdr *)(buffer + (ip->ihl * 4));

  printf("%ld bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
         lenRecv - (ip->ihl * 4), inet_ntoa(destAddress->sin_addr),
         response->un.echo.sequence, ip->ttl, rtt);
}

void printStat(struct stats *stat, struct arguments *arguments) {
  stat->packetLoss = stat->packetTransmitted - stat->packetReceived;
  double avgRTT =
      stat->packetReceived > 0 ? stat->totTimeTrip / stat->packetReceived : 0;
  double stddevRTT =
      stat->packetReceived > 1
          ? sqrt((stat->sqrTimeTrip / stat->packetReceived) - (avgRTT * avgRTT))
          : 0;
  float lossPercent = (stat->packetLoss / (float)stat->packetTransmitted) * 100;

  printf("--- %s ping statistics ---\n%d packets transmitted, %d packets "
         "received, %.0f%% packet loss\n",
         arguments->host, stat->packetTransmitted, stat->packetReceived,
         lossPercent);

  if (stat->packetReceived != 0)
    printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
           stat->minTimeTrip, avgRTT, stat->maxTimeTrip, stddevRTT);
}

void printRespHeader(char *buffer) {
  struct iphdr *ip = (struct iphdr *)buffer;
  // Ip du paquet qui a cause l'erreur (Sur une erreur le routeur qui genere l'erreur envoie un paquet avec Son entete IP
  // (20) ensuite entete ICMP de l'erreur (8) et ensuite le header IP du paquet qui a fait l'erreur donc on recupe ca)
  struct iphdr *orig_ip = (struct iphdr *)(buffer + (ip->ihl * 4) + 8);
  struct icmphdr *orig_icmp =
      (struct icmphdr *)((char *)orig_ip + (orig_ip->ihl * 4));

  printf("IP Hdr Dump:\n ");
  uint16_t *raw = (uint16_t *)orig_ip;
  for (int i = 0; i < (orig_ip->ihl * 2); i++) {
    printf("%04x ", ntohs(raw[i]));
  }
  printf("\n");

  printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst     Data\n");

  char src[INET_ADDRSTRLEN];
  char dst[INET_ADDRSTRLEN];
  struct in_addr saddr = {.s_addr = orig_ip->saddr};
  struct in_addr daddr = {.s_addr = orig_ip->daddr};
  inet_ntop(AF_INET, &saddr, src, sizeof(src));
  inet_ntop(AF_INET, &daddr, dst, sizeof(dst));

  printf(" %1x  %1x  %02x %04x %04x   %1x %04x  %02x  %02x %04x %s  %s \n",
         orig_ip->version, orig_ip->ihl, orig_ip->tos, ntohs(orig_ip->tot_len),
         ntohs(orig_ip->id), (ntohs(orig_ip->frag_off) & 0xe000) >> 13,
         ntohs(orig_ip->frag_off) & 0x1fff, orig_ip->ttl, orig_ip->protocol,
         ntohs(orig_ip->check), src, dst);

  uint16_t icmp_size = ntohs(orig_ip->tot_len) - (orig_ip->ihl * 4);
  printf("ICMP: type %u, code %u, size %u, id 0x%04x, seq 0x%04x\n",
         orig_icmp->type, orig_icmp->code, icmp_size,
         orig_icmp->un.echo.id, orig_icmp->un.echo.sequence);
}