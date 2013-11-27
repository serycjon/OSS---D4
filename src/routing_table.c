#include <stdio.h>
#include "route_cfg_parser.h"
#include "settings.h"

int loadConfigFromFile(char* filename, int local_id){
	int connection_count = MAX_LINKS;
	int local_port;
	TConnection connections[connection_count];
	int i;
	int nodes_count = MAX_NODES;
	int neighbors_counts[nodes_count];
	for(i=0; i<MAX_NODES; i++){
		neighbors_counts[i] = 0;
	}
	int** topology_table;
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

int insertIntoTopologyTable(int from, int to, int** topology_table, int* neighbors_counts, int nodes_count){
	if(neighbors_counts[from] >= MAX_CONNECTIONS){
		fprintf(stderr, "TOPOLOGY_TABLE CONSTRUCTION ERROR: too much connections at %d!\n", from);
		return 0;
	}
	topology_table[from][neighbors_counts[from]++] = to;
	return 1;
}

