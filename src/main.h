#ifndef MAIN_GUARD
#define MAIN_GUARD
#include "settings.h"
#include "topology.h"
#include "routing_table.h"
#include "dynamic_routing.h"

struct shared_mem {
	int local_id;
	int local_port;
	struct real_connection *p_connections;
	TopologyTable *p_topology;
	RoutingTable *p_routing_table;
	NodeStatus **p_status_table;
};

struct mem_and_sfd{
	int sfd;
	struct shared_mem *mem;
};

struct mem_and_buffer{
	char *buf;
	int len;
	struct shared_mem *mem;
};
	

#endif
