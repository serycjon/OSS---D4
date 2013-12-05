#ifndef DYNAMIC_ROUTING_GUARD
#define DYNAMIC_ROUTING_GUARD

#include <time.h>
#include "main.h"
#define OUT_CONN 1
#define IN_CONN 0

struct real_connection{
	char type;
	int id;
	struct sockaddr *addr; // for type == 0 (arrow in)
	int sockfd; // for type == 1 (arrow out)
	time_t last_seen;
	char online; // if timed out, set to 0 => only one NSU
};


int inInit(void* mem);
void outInit(struct shared_mem *mem, Connections out_conns);
void sockListener(void *param);
void helloSender(void *param);
void satanKalous(void *param);

#endif
