#include <stdio.h>
#include "route_cfg_parser.h"
#include "settings.h"

int load_config_from_file(char* filename, int local_id){
	int connection_count = MAX_LINKS;
	int local_port;
	TConnection connections[connection_count];
	int nodes_count = MAX_NODES;
	int neighbors_counts[nodes_count];
	int** topology_table;
	int i;
	topology_table = (int **) malloc(MAX_NODES * sizeof(int *));
	for(i=0; i<MAX_NODES; i++){
		topology_table[i] = (int *) malloc(MAX_CONNECTIONS * sizeof(int));
	}
	int result = parseRouteConfiguration(filename, local_id, &local_port, &connection_count, connections,
			&nodes_count, neighbors_counts, topology_table);
	if (!result) {
		return 1;
	}

	return 0;
}
