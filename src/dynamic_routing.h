#ifndef DYNAMIC_ROUTING_GUARD
#define DYNAMIC_ROUTING_GUARD

#include <time.h>
#include "main.h"

#define RETRY 0
#define NORETRY 1

struct real_connection{
	char type;
	int id;
	struct sockaddr *addr; // for type == IN_CONN
	int sockfd;	
	time_t last_seen;
	char online; // if timed out, set to OFFLINE => only one NSU
};


void helloSender(void *param);
void deathChecker(void *param);
void reactToStateChange(int id, int new_state, struct shared_mem *mem);
void sendNSU(int id, int new_state, struct shared_mem *mem);
void sendToNeighbours(int not_to, char *packet, int len, struct shared_mem *mem);
int sendToId(int dest_id, char *packet, int len, struct shared_mem *p_mem, int retry);


#endif
