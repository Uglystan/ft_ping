#ifndef CREATE_PACKET_H
# define CREATE_PACKET_H

# include "main.h"

uint16_t checksum(void *temp, int len);
void createPacket(struct ping_pkt *packet);

#endif
