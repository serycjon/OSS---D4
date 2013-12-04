#ifndef PACKETS_GUARD
#define PACKETS_GUARD

#include "main.h"
#define T_MSG 0
#define T_HELLO 1
#define T_NSU 2
#define T_DD 3

char *formHelloPacket(int id, int *len);
void packetParser(void *parameter);

void parseMsg(struct mem_and_buffer *params);
void parseHello(struct mem_and_buffer *params);
void parseNSU(struct mem_and_buffer *params);
void parseDD(struct mem_and_buffer *params);

#endif
