#ifndef MAIN_GUARD
#define MAIN_GUARD
#include "settings.h"
#include "topology.h"
#include "routing_table.h"
#include "dynamic_routing.h"

struct shared_mem {
	struct real_connection *p_connections;
	TopologyTable *p_topology;
	RoutingTable *p_routing_table;
	NodeStatus **p_status_table;
};

#endif
