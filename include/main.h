#ifndef MAIN_H
# define MAIN_H

# define _GNU_SOURCE

# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <stdbool.h>
# include <string.h>
# include <strings.h>
# include <errno.h>
# include <signal.h>
# include <math.h>
# include <fcntl.h>
# include <ctype.h>
# include <stdint.h>
# include <sys/types.h>
# include <sys/time.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <netinet/ip.h>
# include <netinet/ip_icmp.h>

# define PKT_SIZE 64

extern int stop;

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