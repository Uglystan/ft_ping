#ifndef PRINT_UTILS_H
# define PRINT_UTILS_H

#include <stdio.h>
#include "main.h"

void printHelp(void);
void printHeader(struct arguments *arguments);
void printData(char *buffer, double rtt, struct sockaddr_in *destAddress, ssize_t lenRecv);
void printStat(struct stats *stat, struct arguments *arguments);
void printRespHeader(char *buffer);

#endif