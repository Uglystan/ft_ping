#ifndef MAIN_H
# define MAIN_H

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netdb.h>
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>
#include <ctype.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <signal.h>
#include <strings.h>
#include <math.h>
#define PKT_SIZE 64

struct arguments {
    bool verboseIsEnable;
    bool helpIsEnable;
    char *host;
    char addressPrintable[INET_ADDRSTRLEN];
    struct sockaddr_in destAddress;
};

struct ping_pkt {
    struct icmphdr hdr;                             // 8 octets
    char msg[PKT_SIZE - sizeof(struct icmphdr)]; // 56 octets de données
};

struct stats {
    int packetTransmitted;
    int packetReceived;
    int packetLoss;
    double minTimeTrip;
    double totTimeTrip;
    double maxTimeTrip;
    double sqrTimeTrip;
};

#endif