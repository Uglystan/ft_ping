#include "createPacket.h"

/*
Le checksum permet de verifier qu'il n'y a pas eu de corruption sur le packet.
    - Le principe c'est d'additionner 2 octects par 2 octects (uint16_t) tout le packet (variable cheksum a 0)
      Ensuite le checksum etant sur 16bits et l'addition ayant pu creer un nombre plus grand on garde uniquement
      on decoupe en 2 partie de 16bits (pour les bits du haut sum >> 16) qu'on additione (bits du bas sum & 0xFFFF)
      il se peut qu'il y'a encore une depassement (si de base on avait 0x0001FFFF) donc on ne garde que les bits du haut (0X0001)
      puis on inverse le resultat. La programme qui recoit fait exactement la meme operation et compare avec notre checksum.
      si c'est le meme pas de corruption sinon le paquet est corrompu.
*/
uint16_t checksum(void *temp, int len) {
  uint16_t *buf = temp;
  unsigned int sum = 0;
  uint16_t result;

  for (sum = 0; len > 1; len -= 2)
    sum += *buf++;

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  result = ~sum;
  return result;
}

void createPacket(struct ping_pkt *packet) {
  packet->hdr.type = ICMP_ECHO;
  packet->hdr.code = 0;
  packet->hdr.un.echo.id = getpid() & 0xFFFF;
  packet->hdr.un.echo.sequence = 0;

  // Remplir les 56 octets de données avec un motif
  for (size_t i = 0; i < sizeof(packet->msg); i++) {
    packet->msg[i] = (char)i;
  }
}