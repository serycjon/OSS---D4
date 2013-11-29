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

RoutingTableEntry** createRoutingTable (TopologyTable* topology, int node_ID, NodeStatus* status_table){
	StackEntry** stack = (StackEntry**) malloc(MAX_NODES * sizeof(StackEntry*));
	RoutingTableEntry** routing_table = (RoutingTableEntry**) malloc(MAX_NODES * sizeof(RoutingTableEntry*));
	int visited_nodes, i, last;
	last = 0;
	visited_nodes = 0;

	for(i=0; i<MAX_NODES;i++){
		routing_table[i] = NULL;
	}

	RoutingTableEntry* first_node;		//add IP and port!
	routing_table[node_ID] = first_node; 
	
	for(i=0; i<topology->neighbors_counts[node_ID]; i++){			//adding close neighbors into stack and routing_table
			int new_node_ID  = topology->table[node_ID][i];
			if(routing_table[new_node_ID] == NULL && status_table[new_node_ID] == ONLINE){
				StackEntry* new;
				RoutingTableEntry* new_entry; //add IP and port!
				new->final_node_ID=new_node_ID;
				new->first_hop_node_ID = new_node_ID;	
				stack[last++] = new;
				routing_table[new_node_ID] = new_entry;
			}

	while (visited_nodes <= topology->nodes_count && visited_nodes<=last){	//untill there are some accesible unattended nodes
		int actual_node_ID = stack[visited_nodes]->final_node_ID;
		
		for(i=0; i<topology->neighbors_counts[actual_node_ID]; i++){	//check actual node neighbors
			int new_node_ID  = topology->table[actual_node_ID][i];
			if(routing_table[new_node_ID] == NULL && status_table[new_node_ID] == ONLINE){
				StackEntry* new;
				RoutingTableEntry* new_entry; //add IP and port!
				new->final_node_ID=new_node_ID;
				new->first_hop_node_ID = stack[visited_nodes]->first_hop_node_ID;
				stack[last++] = new;
				routing_table[new_node_ID] = new_entry;
			}			
		}
		visited_nodes++;
	}

	free(stack);
	stack = NULL;

	return routing_table;
}


