#ifndef ROUTING_TABLE_GUARD
#define ROUTING_TABLE_GUARD

#include "settings.h"
#include "topology.h"
#include "route_cfg_parser.h"

typedef enum {
	ONLINE,
	OFFLINE
} NodeStatus;


typedef struct routing_table_entry {
	int next_hop_id;
	int next_hop_port;
	char next_hop_ip[IP_ADDRESS_MAX_LENGTH];
} RoutingTableEntry;

typedef struct routing_table {
	int size;
	RoutingTableEntry* table;
} RoutingTable;

typedef struct stack_entry {
	int final_node_ID;
	int first_hop_node_ID;
} StackEntry, *Stack;

int initRouting(char* filename, int local_id);

RoutingTable createRoutingTable (TopologyTable topology, Connections connections, int node_ID, NodeStatus* status_table);
int idToIndex(int id);
int indexToId(int id);
void showRoutingTable(RoutingTable routing_table);
void setAddressById(int id, RoutingTableEntry* entry, Connections connections);
void showStatusTable(int status_table_size, NodeStatus* state_table);

#endif
