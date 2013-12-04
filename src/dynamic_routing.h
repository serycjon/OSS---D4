#ifndef DYNAMIC_ROUTING_GUARD
#define DYNAMIC_ROUTING_GUARD

#include <time.h>
#define OUT_CONN 1
#define IN_CONN 0

struct real_connection{
	char type;
	int id;
	struct addrinfo *addrinfo; // for type == 0 (arrow in)
	int sockfd; // for type == 1 (arrow out)
	clock_t last_seen;
	char online; // if timed out, set to 0 => only one NSU
};

struct mem_and_sfd{
	int sfd;
	struct shared_mem *mem;
};
	

int routingListen(void* port_number);
int sayHello(Connections conns);
void outInit(struct shared_mem *mem, Connections out_conns);
void sockListener(void *param);

#endif
