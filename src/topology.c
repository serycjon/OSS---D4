#include <stdio.h>
#include "topology.h"
#include "settings.h"

int insertIntoTopologyTable(int from, int to, TopologyTable* p_topology){
	if(p_topology->neighbors_counts[from] >= MAX_CONNECTIONS){
		fprintf(stderr, "TOPOLOGY_TABLE CONSTRUCTION ERROR: too much connections at %d!\n", from);
		return FAILURE;
	}else if(p_topology->neighbors_counts[to] >= MAX_CONNECTIONS){
		fprintf(stderr, "TOPOLOGY_TABLE CONSTRUCTION ERROR: too much connections at %d!\n", to);
		return FAILURE;
	}
	int i;
	int already_there = 0;
	for(i=0; i < p_topology->neighbors_counts[from]; i++){
		if(p_topology->table[from][i] == to){
			already_there = 1;
		}
	}
	if(!already_there){
		p_topology->table[from][p_topology->neighbors_counts[from]++] = to;
	}
	already_there = 0;
	for(i=0; i < p_topology->neighbors_counts[to]; i++){
		if(p_topology->table[to][i] == from){
			already_there = 1;
		}
	}
	if(!already_there){
		p_topology->table[to][p_topology->neighbors_counts[to]++] = from;
	}
	return SUCCESS;
}

void showTopology(TopologyTable topology){
	printf("--------------------------------\n"
		"Network topology:\n"
		"--------------------------------\n");
	int i, j;
	for(i = MIN_ID; i < topology.nodes_count; i++){
		for(j = 0; j < topology.neighbors_counts[i]; j++){
			printf("Connection: %d -> %d\n", i, topology.table[i][j]);
		}
		printf("-------\n");
	}
	printf("--------------------------------\n");
}
