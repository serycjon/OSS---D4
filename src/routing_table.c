#include <stdio.h>
#include "route_cfg_parser.h"
#include "routing_table.h"
#include "settings.h"

int initRouting(char* filename, int local_id){
	int connection_count = MAX_LINKS;
	int local_port;
	TConnection connections[connection_count];
	int nodes_count = MAX_NODES;
	int neighbors_counts[nodes_count];
	int** topology_table;
	//init topology table
	int i;
	for(i=0; i<MAX_NODES; i++){
		neighbors_counts[i] = 0;
	}
	topology_table = (int **) malloc(MAX_NODES * sizeof(int *));
	for(i=0; i<MAX_NODES; i++){
		topology_table[i] = (int *) malloc(MAX_CONNECTIONS * sizeof(int));
	}
	//parse config file
	int result = parseRouteConfiguration(filename, local_id, &local_port, &connection_count, connections,
			&nodes_count, neighbors_counts, topology_table);
	if (result == FAILURE) {
		return FAILURE;
	}
	

	show_topology(nodes_count, neighbors_counts, topology_table);

	

	return SUCCESS;
}

int insertIntoTopologyTable(int from, int to, int** topology_table, int* neighbors_counts, int nodes_count){
	if(neighbors_counts[from] >= MAX_CONNECTIONS){
		fprintf(stderr, "TOPOLOGY_TABLE CONSTRUCTION ERROR: too much connections at %d!\n", from);
		return FAILURE;
	}else if(neighbors_counts[to] >= MAX_CONNECTIONS){
		fprintf(stderr, "TOPOLOGY_TABLE CONSTRUCTION ERROR: too much connections at %d!\n", to);
		return FAILURE;
	}
	int i;
	int already_there = 0;
	for(i=0; i < neighbors_counts[from]; i++){
		if(topology_table[from][i] == to){
			already_there = 1;
		}
	}
	if(!already_there){
		topology_table[from][neighbors_counts[from]++] = to;
	}
	already_there = 0;
	for(i=0; i < neighbors_counts[to]; i++){
		if(topology_table[to][i] == from){
			already_there = 1;
		}
	}
	if(!already_there){
		topology_table[to][neighbors_counts[to]++] = from;
	}
	return SUCCESS;
}

void showTopology(int nodes_count, int* neighbors_counts, int** topology_table){
	printf("--------------------------------\n"
		"Network topology:\n"
		"--------------------------------\n");
	int i, j;
	for(i = 0; i < nodes_count; i++){
		for(j = 0; j < neighbors_counts[i]; j++){
			printf("Connection: %d -> %d\n", i, topology_table[i][j]);
		}
		// printf("-------\n");
	}
	printf("--------------------------------\n");
}
