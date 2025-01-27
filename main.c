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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>

struct arguments {
    bool verboseIsEnable;
    bool helpIsEnable;
    char addressPrintable[INET_ADDRSTRLEN];
    unsigned char address[sizeof(struct in_addr)];
};

//Pour trouver l'ip en local si pas trouver il faut cherche la paserelle dans le fichier proc/net/route la destination sera 0.0.0.0 pour la paserelle par default
bool isOnSameNetwork (char *address, struct ifaddrs *interface) {
    struct sockaddr_in *temp = (struct sockaddr_in *) interface->ifa_addr;

    u_int32_t netIpInterface = temp->sin_addr.s_addr;

    temp = (struct sockaddr_in *)interface->ifa_netmask;

    u_int32_t netMaskInterface = temp->sin_addr.s_addr;

    u_int32_t ip;
    inet_pton(AF_INET, address, &ip);

    if ((netIpInterface & netMaskInterface) == (ip & netMaskInterface))
        return (true);
    return (false);
}

bool selectInterface (char *address) {
    struct ifaddrs *interfaceList;

    printf("%s\n\n\n", address);


    if (getifaddrs(&interfaceList) == -1)
        return (perror("Error: "), false);

    for (struct ifaddrs *temp = interfaceList; temp != NULL; temp = temp->ifa_next) {
        printf("Interface name : %s\n", temp->ifa_name);
        printf("Interface family : %d\n", temp->ifa_addr->sa_family);
        if (temp->ifa_addr->sa_family == AF_INET) {
            if (isOnSameNetwork(address, temp)) {
                printf("Find");
            }
        }
    }

    freeifaddrs(interfaceList);
    return (true);
}

void printHelp(void) {
    printf("Usage: ping [OPTION...] HOST ...\nSend ICMP ECHO_REQUEST packets to network hosts.\n\nOptions valid for all request types:\n\n -v, --verbose              verbose output\n");
}

bool parseArg(int argc, char **argv, struct arguments *arguments) {

    if (argc == 2) {
        if (strcmp(argv[1], "-?") == 0) {
            return (arguments->verboseIsEnable = true, true);
        }
        if (!inet_pton(AF_INET, argv[1], arguments->address))
            return (printf("Error: invalid IP address\n"), false);
    }
    else if (argc == 3) {
        if (strcmp(argv[1], "-?") != 0 && strcmp(argv[1], "-v") != 0)
            return (printf("Error: invalid options\n"), false);

        if (strcmp(argv[1], "-?") == 0)
            arguments->helpIsEnable = true;

        if (strcmp(argv[1], "-v") == 0)
            arguments->verboseIsEnable = true;

        if (inet_pton(AF_INET, argv[2], arguments->address)) {
            if (inet_ntop(AF_INET, arguments->address, arguments->addressPrintable, INET_ADDRSTRLEN) == NULL)
                return (perror("Error: "), false);
        }
        else
            return (printf("Error: invalid IP address\n"), false);
    }
    else
        return (printf("Error: argument number incorrect\n"), false);
    return (true);
}

int main(int argc, char **argv) {
    struct arguments arguments;

    if (!parseArg(argc, argv, &arguments))
        return (1);
    if (arguments.helpIsEnable)
        return(printHelp(), 1);
    selectInterface(arguments.addressPrintable);

    printf("OK, adressPrintable -> %s. adress -> %s\n", arguments.addressPrintable, arguments.address);
    return(0);
}