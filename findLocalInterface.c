#include "main.h"

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

bool findLocalInterface (char *address, struct ifaddrs *interface) {
    struct ifaddrs *interfaceList;

    printf("%s\n\n\n", address);


    if (getifaddrs(&interfaceList) == -1)
        return (perror("Error: "), false);

    for (struct ifaddrs *temp = interfaceList; temp != NULL; temp = temp->ifa_next) {
        printf("Interface name : %s\n", temp->ifa_name);
        printf("Interface family : %d\n", temp->ifa_addr->sa_family);
        if (temp->ifa_addr->sa_family == AF_INET) {
            if (isOnSameNetwork(address, temp)) {
                memcpy(interface, temp, sizeof(struct ifaddrs));
                interface->ifa_next = NULL;
                freeifaddrs(interfaceList);
                return (true);
            }
        }
    }

    freeifaddrs(interfaceList);
    return (false);
}