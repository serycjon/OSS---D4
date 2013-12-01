#include <stdio.h>
#include <string.h>
#include "route_cfg_parser.h"
#include "routing_table.h"
#include "topology.h"
#include "settings.h"

int initRouting(char* filename, int local_id){
	int local_port;
	TopologyTable topology;

	TConnection local_conns[MAX_CONNECTIONS];
	Connections local_connections;
	local_connections.count = MAX_CONNECTIONS;
	local_connections.array =  local_conns;

	TConnection bidir_conns[MAX_CONNECTIONS];
	Connections bidir_connections;
	bidir_connections.count = MAX_CONNECTIONS;
	bidir_connections.array =  bidir_conns;

	int result = parseRouteConfiguration(filename, local_id, &local_port, &bidir_connections, &local_connections, &topology);

	showConnections(bidir_connections);

	if (result == FAILURE) {
		return FAILURE;
	}


	showTopology(topology);
	// Dummy status_table
	NodeStatus status_table[MAX_NODES];
	int i;
	for(i = 0; i < MAX_NODES; i++){
		status_table[i] = ONLINE;
	}
	return SUCCESS;


	RoutingTable routing_table = createRoutingTable(topology, bidir_connections, local_id, status_table);
	showRoutingTable(routing_table);

	return SUCCESS;
}

RoutingTable createRoutingTable (TopologyTable topology, Connections connections, int node_ID, NodeStatus* status_table)
{
	StackEntry* stack = (StackEntry*) malloc((topology.nodes_count+1) * sizeof(StackEntry));
	RoutingTable routing_table;
	routing_table.table = (RoutingTableEntry*) malloc((topology.nodes_count+1) * sizeof(RoutingTableEntry));
	routing_table.size = topology.nodes_count;
	int visited_nodes, i, last;
	last = 0;
	visited_nodes = 0;


	for(i=0; i<topology.nodes_count;i++){
		RoutingTableEntry no_path;
		no_path.next_hop_port = -1;
		no_path.next_hop_id = -1;
		strcpy(no_path.next_hop_ip, "none");
		routing_table.table[i] = no_path;
	}

	// routing to myself probably won't be needed
	RoutingTableEntry first_node;		//add IP and port!
	showConnections(connections);
	setAddressById(node_ID, &first_node, connections);
	printf("first node: %d; %d; %s\n", first_node.next_hop_id, first_node.next_hop_port, first_node.next_hop_ip);
	routing_table.table[idToIndex(node_ID)] = first_node; 

	showRoutingTable(routing_table);

	for(i=0; i<topology.neighbors_counts[idToIndex(node_ID)]; i++){			//adding close neighbors into stack and routing_table
		int new_node_ID  = topology.table[idToIndex(node_ID)][i];
		// showRoutingTable(routing_table);
		// printf("debug: %d - new_node_id 59\n", new_node_ID);
		if(routing_table.table[idToIndex(new_node_ID)].next_hop_port == -1 &&
				status_table[idToIndex(new_node_ID)] == ONLINE){
			StackEntry new;
			RoutingTableEntry new_entry; //add IP and port!
			setAddressById(new_node_ID, &new_entry, connections);
			new.final_node_ID=new_node_ID;
			new.first_hop_node_ID = new_node_ID;	
			stack[last++] = new;
			routing_table.table[idToIndex(new_node_ID)] = new_entry;
		}
	}
	while (visited_nodes <= topology.nodes_count && visited_nodes<=last){	//untill there are some accesible unattended nodes
		int actual_node_ID = stack[visited_nodes].final_node_ID;
		printf("%d -> %d\n", actual_node_ID, idToIndex(actual_node_ID));
		for(i=0; i<topology.neighbors_counts[idToIndex(actual_node_ID)]; i++){	//check actual node neighbors
			int new_node_ID  = topology.table[idToIndex(actual_node_ID)][i];
			if(routing_table.table[idToIndex(new_node_ID)].next_hop_port == -1 && status_table[idToIndex(new_node_ID)] == ONLINE){
				StackEntry new;
				RoutingTableEntry new_entry; //add IP and port!
				setAddressById(new_node_ID, &new_entry, connections);
				new.final_node_ID=new_node_ID;
				new.first_hop_node_ID = stack[visited_nodes].first_hop_node_ID;
				stack[last++] = new;
				routing_table.table[idToIndex(new_node_ID)] = new_entry;
			}			
		}
		visited_nodes++;
	}

	free(stack);
	stack = NULL;

	return routing_table;
}

int idToIndex(int id)
{
	return id; // - MIN_ID;
}

int indexToId(int index){
	return index; // + MIN_ID;
}

void showRoutingTable(RoutingTable routing_table)
{
	printf("---------------\n");
	printf("Routing table\n");
	printf("---------------\n");
	int i;
	for(i=0; i<routing_table.size; i++){
		printf("route to %d via %d\n", i, routing_table.table[i].next_hop_id);
	}
	printf("---------------\n");
}

void setAddressById(int id, RoutingTableEntry* entry, Connections connections)
{
	int i;
	for(i=0; i<connections.count; i++){
		if(connections.array[i].id == id){
			entry->next_hop_id = connections.array[i].id;
			entry->next_hop_port = connections.array[i].port;
			strcpy(entry->next_hop_ip, connections.array[i].ip_address);
		}
	}
}
