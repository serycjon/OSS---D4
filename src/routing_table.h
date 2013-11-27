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

int loadConfigFromFile(char* filename, int local_id);

int insertIntoTopologyTable(int from, int to, int** topology_table, int* neighbors_counts, int nodes_count);
#endif
