#include "main.h"

//Pour trouver l'ip en local si pas trouver il faut cherche la paserelle dans le fichier proc/net/route la destination sera 0.0.0.0 pour la paserelle par default
// enp0s3  00000000        0202000A        0003    0       0       100     00000000        0       0       0                                                                           
// enp0s3  0002000A        00000000        0001    0       0       100     00FFFFFF        0       0       0                                                                           
// docker0 000011AC        00000000        0001    0       0       0       0000FFFF        0       0       0

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

        if (inet_ntop(AF_INET, arguments->address, arguments->addressPrintable, INET_ADDRSTRLEN) == NULL)
            return (perror("Error: "), false);
    }
    else if (argc == 3) {
        if (strcmp(argv[1], "-?") != 0 && strcmp(argv[1], "-v") != 0)
            return (printf("Error: invalid options\n"), false);

        if (strcmp(argv[1], "-?") == 0)
            arguments->helpIsEnable = true;

        if (strcmp(argv[1], "-v") == 0)
            arguments->verboseIsEnable = true;

        if (inet_pton(AF_INET, argv[2], arguments->address) != 1)
            return (printf("Error: invalid IP address\n"), false);
        
        if (inet_ntop(AF_INET, arguments->address, arguments->addressPrintable, INET_ADDRSTRLEN) == NULL)
            return (perror("Error: "), false);
    }
    else
        return (printf("Error: argument number incorrect\n"), false);
    // printf("address printabe -> %s\n", inet_ntop(AF_INET, arguments->address, arguments->addressPrintable, INET_ADDRSTRLEN));
    return (true);
}

int main(int argc, char **argv) {
    struct arguments arguments;
    struct ifaddrs interface;
    memset(&arguments, 0, sizeof(struct arguments));
    memset(&interface, 0, sizeof(struct ifaddrs));

    if (!parseArg(argc, argv, &arguments))
        return (1);
    if (arguments.helpIsEnable)
        return(printHelp(), 1);
    printf("OK, adressPrintable -> %s. adress -> %s\n", arguments.addressPrintable, arguments.address);
    if (findLocalInterface(arguments.addressPrintable, &interface))
        printf("Interface trouve en local -> %s\n", interface.ifa_name);
    else if (findDefaultInterface(&interface))
        printf("Interface par default trouve -> %s\n", interface.ifa_name);

    return(0);
}