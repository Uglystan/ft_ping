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

char *findLocalInterface (char *address) {
    struct ifaddrs *interfaceList;
    char *name;

    if (getifaddrs(&interfaceList) == -1)
        return (perror("Error: "), NULL);

    for (struct ifaddrs *temp = interfaceList; temp != NULL; temp = temp->ifa_next) {
        if (temp->ifa_addr->sa_family == AF_INET) {
            if (isOnSameNetwork(address, temp)) {
                name = strdup(temp->ifa_name);
                // name = malloc(strlen(temp->ifa_name) * sizeof(char));
                // strcpy(name, temp->ifa_name);
                freeifaddrs(interfaceList);
                return (name);
            }
        }
    }

    freeifaddrs(interfaceList);
    return (NULL);
}