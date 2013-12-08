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
} RoutingTableEntry;

typedef struct routing_table {
	int size;
	RoutingTableEntry* table;
} RoutingTable;

typedef struct stack_entry {
	int final_node_ID;
	int first_hop_node_ID;
} QueueEntry, *Queue;

struct shared_mem;

int initRouting(char* filename, int local_id, struct shared_mem *mem);
void createRoutingTable (struct shared_mem *mem);
int idToIndex(int id);
int indexToId(int id);
void showRoutingTable(struct shared_mem *mem);
/* //discontinued
 * void setAddressById(int id, RoutingTableEntry* entry, Connections connections);
 */
void showStatusTable(int status_table_size, NodeStatus* state_table);

#endif
