#include "main.h"

bool isDefaultInterface(char *line) {
    size_t i = 0;
    size_t j = 0;
    char dest[9] = {0};

    while (isalnum(line[i]) != 0)
        i++;
    while (isalnum(line[i]) == 0)
        i++;
    for(j = i; line[j] && isalnum(line[j]) != 0 && j - i < 8; j++) {
        dest[j - i] = line[j];
    }
    dest[j - i] = '\0';

    if (strcmp("00000000", dest) == 0)
        return (true);
    return (false);
}

char *getDefaultInterface(char *line) {
    struct ifaddrs *interfaces = {0};
    char *name;
    size_t i = 0;
    size_t j = 0;

    for (; isalnum(line[i]) != 0; i++) {
    }

    if ((name = malloc((i + 1) * sizeof(char))) == NULL)
        return (printf("Error: Malloc failed"), NULL);

    for (; isalnum(line[j]) != 0; j++) {
        name[j] = line[j];
    }

    name[j] = '\0';

    if (getifaddrs(&interfaces) != 0) {
        free(name);
        return (perror("Error: "), NULL);
    }

    for (struct ifaddrs *temp = interfaces; temp != NULL; temp = temp->ifa_next) {
        if (strcmp(name, temp->ifa_name) == 0) {
            freeifaddrs(interfaces);
            printf("%s\n", name);
            return (name);
        }
    }
    freeifaddrs(interfaces);
    free(name);
    return (NULL);
}

char *findDefaultInterface() {
    FILE *fp = fopen("/proc/net/route", "r");
    char *line = NULL;
    size_t size = 0;

    if (fp == NULL)
        return (printf("Error: File /proc/net/route not found\n"), NULL);

    while (getline(&line, &size, fp) != -1) {
        if (isDefaultInterface(line)) {
            char *name = getDefaultInterface(line);
            fclose(fp);
            free(line);
            return (name);
        }
    }

    fclose(fp);
    free(line);
    return (NULL);
}