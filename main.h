#ifndef MAIN_H
# define MAIN_H

#include <stdio.h>
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

struct arguments {
    bool verboseIsEnable;
    bool helpIsEnable;
    char addressPrintable[INET_ADDRSTRLEN];
    unsigned char address[sizeof(struct in_addr)];
};

bool findLocalInterface (char *address, struct ifaddrs *interface);

#endif