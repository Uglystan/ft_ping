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

struct arguments {
    bool verboseIsEnable;
    bool helpIsEnable;
    char addressPrintable[INET_ADDRSTRLEN];
    unsigned char address[sizeof(struct in_addr)];
};

char  *findLocalInterface (char *address);
char *findDefaultInterface();

#endif