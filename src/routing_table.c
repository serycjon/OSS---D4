#include <stdio.h>
#include "route_cfg_parser.h"
#include "settings.h"

int load_config_from_file(char* filename, int local_id){
	int connection_count = MAX_CONNECTIONS;
	int local_port;
	TConnection connections[connection_count];
	int nodes_count = MAX_NODES;
	int neighbors_counts[nodes_count];
	int* topology_table[nodes_count];
	int result = parseRouteConfiguration(filename, local_id, &local_port, &connection_count, connections,
						&nodes_count, neighbors_counts, topology_table);
	if (result) {
		printf("OK, local port: %d\n", local_port);

		if (connection_count > 0) {
			int i;
			for (i=0; i<connection_count; i++) {
				printf("Connection to node %d at %s%s%d\n", connections[i].id, connections[i].ip_address, (connections[i].ip_address[0])?":":"port ", connections[i].port);
			}
		} else {
			printf("No connections from this node\n");
		}
	} else {
		printf("ERROR\n");
		return 1;
	}
	return 0;
}
