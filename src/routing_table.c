#include <stdio.h>
#include "route_cfg_parser.h"
#include "routing_table.h"
#include "topology.h"
#include "settings.h"

int initRouting(char* filename, int local_id){
	int connection_count = MAX_LINKS;
	int local_port;
	TConnection connections[connection_count];
	TopologyTable topology;
		//parse config file
	int result = parseRouteConfiguration(filename, local_id, &local_port, &connection_count, connections, &topology);
	if (result == FAILURE) {
		return FAILURE;
	}
	

	showTopology(topology);

	return SUCCESS;
}


