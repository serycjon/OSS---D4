#ifndef NETWORK_GUARD
#define NETWORK_GUARD

#include <time.h>
#include "main.h"

#define OUT_CONN 1
#define IN_CONN 0

void *get_in_addr(struct sockaddr *sa);
void *get_in_port(struct sockaddr *sa);
int inInit(void* mem);
void outInit(struct shared_mem *mem, Connections out_conns);
void sockListener(void *param);
void sendToNeighbour(int dest_id, char *packet, int len, struct shared_mem *p_mem);

#endif
