#ifndef ROUTING_TABLE_GUARD
#define ROUTING_TABLE_GUARD

#include "settings.h"

typedef enum {
	ONLINE,
	OFFLINE
} NodeStatus;


typedef struct routing_table_entry {
	int next_hop_port;
	char next_hop_ip[IP_ADDRESS_MAX_LENGTH];
} RoutingTableEntry;

int initRouting(char* filename, int local_id);

#endif
