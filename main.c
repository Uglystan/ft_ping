#include "main.h"

//Pour trouver l'ip en local si pas trouver il faut cherche la paserelle dans le fichier proc/net/route la destination sera 0.0.0.0 pour la paserelle par default
// enp0s3  00000000        0202000A        0003    0       0       100     00000000        0       0       0                                                                           
// enp0s3  0002000A        00000000        0001    0       0       100     00FFFFFF        0       0       0                                                                           
// docker0 000011AC        00000000        0001    0       0       0       0000FFFF        0       0       0

int stop = 0;

void printHelp(void) {
    printf("Usage: ping [OPTION...] HOST ...\nSend ICMP ECHO_REQUEST packets to network hosts.\n\nOptions valid for all request types:\n\n -v, --verbose              verbose output\n");
}

bool reverseDns(char *name) {
    struct addrinfo *address; 
    getaddrinfo(name, NULL, NULL, &address);

    for (struct addrinfo *temp = address; temp != NULL; temp = temp->next){
        printf(temp.)
    }
}

bool parseArg(int argc, char **argv, struct arguments *arguments) {

    if (argc == 2) {
        if (strcmp(argv[1], "-?") == 0) {
            return (arguments->verboseIsEnable = true, true);
        }
        if (!inet_pton(AF_INET, argv[1], arguments->address))
            if (!reverseDns(argv[1]))
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

// Checksum permet de verfifier qu'il n'y a pas eu de corruption du packet durant l'envoie on additione tout les elements du packet
// ce qui donne un nombre et ensuite ou l'invers "~sum". Quand i sera recu par le destinataire il va add tout les champs du packet dont le checksum (reverse)
// et il doit s'attendre a trouver 0xFFFF
unsigned short checksum(void *temp, int len) {
    unsigned short *buf = temp;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char*)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

void createPacket(struct icmphdr *packet) {
    packet->type = ICMP_ECHO;
    packet->code = 0;
    packet->un.echo.id = getpid();
    packet->un.echo.sequence = 1;
}

bool createDestAddress(struct arguments *arguments, struct sockaddr_in *destAddress) {
    destAddress->sin_family = AF_INET;
    if (inet_pton(AF_INET, arguments->addressPrintable, &destAddress->sin_addr) != 1)
        return (perror("Error: "), false);
    destAddress->sin_port = 0;
    return (true);
}

void printData(char *buffer, double rtt) {
    struct iphdr *ip = (struct iphdr *) buffer;
    struct icmphdr *response = (struct icmphdr *)(buffer + (ip->ihl * 4));

    printf("%ld bytes\n", sizeof(buffer) * sizeof(char *));
    printf("icmp_seq=%d\n", response->un.echo.sequence);
    printf("time=%.3f ms\n", rtt);
    printf("ttl=%d\n", ip->ttl);
}

bool sendPacket(struct sockaddr_in *destAddress, struct sockaddr_in *srcAddress, int sock) {
    struct icmphdr packet;
    struct timeval startTv, endTv;
    ssize_t lenSend, lenRecv = 0;
    char buffer[IP_MAXPACKET];
    socklen_t size = sizeof(struct sockaddr_in);
    
    memset(&packet, 0, sizeof(struct icmphdr));
    createPacket(&packet);

    while (!stop) {
        packet.checksum = 0;
        packet.checksum = checksum(&packet, sizeof(struct icmphdr));
        lenSend = sendto(sock, &packet, sizeof(struct icmphdr), 0, (struct sockaddr *) destAddress, size);
        gettimeofday(&startTv, NULL);
        lenRecv = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) srcAddress, &size);
        gettimeofday(&endTv, NULL);

        double rtt = (endTv.tv_sec - startTv.tv_sec) * 1000.0 + (endTv.tv_usec - startTv.tv_usec) / 1000.0;

        printData(buffer, rtt);

        if (lenSend < 0 || lenRecv < 0)
            return (perror("Error: "), false);

        packet.un.echo.sequence++;
        sleep(1);
    }

    return (true);
}

bool ping(struct arguments *arguments) {
    struct sockaddr_in destAddress;
    struct sockaddr_in srcAddress;
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    struct timeval timeout;

    //Init d'un tiimeout pour recvfrom si pas de reponse au bout de 2sec
    timeout.tv_sec = 2;   
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memset(&destAddress, 0, sizeof(struct sockaddr_in));

    if (createDestAddress(arguments, &destAddress) == false)
        return (false);
    sendPacket(&destAddress, &srcAddress, sock);

    return (true);
}

//faire les moyennes, resolution DNS (google.com), Si ping marche pas
int main(int argc, char **argv) {
    setSignalAction();
    struct arguments arguments;
    memset(&arguments, 0, sizeof(struct arguments));

    if (!parseArg(argc, argv, &arguments))
        return (1);
    if (arguments.helpIsEnable)
        return(printHelp(), 1);
    ping(&arguments);
    return(0);
}