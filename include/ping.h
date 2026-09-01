#ifndef PING_H
# define PING_H

# include "main.h"
# include "printUtils.h"
# include "createPacket.h"

bool ping(struct arguments *arguments);
bool sendPacket(struct sockaddr_in *srcAddress, int sock, struct arguments *arguments);

#endif