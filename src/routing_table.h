#ifndef ROUTING_TABLE_GUARD
#define ROUTING_TABLE_GUARD

#include "settings.h"

typedef enum {
	ONLINE,
	OFFLINE
} RouteStatus;

typedef struct routing_table_entry {
	RouteStatus status;
	int next_hop_port;
	char next_hop_ip[IP_ADDRESS_MAX_LENGTH];
} RoutingTableEntry;

int load_config_from_file(char* filename, int local_id);

#endif
