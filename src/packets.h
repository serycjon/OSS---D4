#ifndef PACKETS_GUARD
#define PACKETS_GUARD

#include "main.h"
#define T_MSG 0
#define T_HELLO 1
#define T_NSU 2
#define T_DD 3

char *formHelloPacket(int id, int *len);
char *formMsgPacket(int source_id, int dest_id, char* text, int *len);
void packetParser(void *parameter);

void parseMsg(struct mem_and_buffer_and_sfd *params);
void parseHello(struct mem_and_buffer_and_sfd *params);
void parseNSU(struct mem_and_buffer_and_sfd *params);
void parseDD(struct mem_and_buffer_and_sfd *params);

#endif
