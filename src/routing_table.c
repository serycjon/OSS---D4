#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "route_cfg_parser.h"
#include "routing_table.h"
#include "topology.h"
#include "settings.h"
#include "dynamic_routing.h"

int initRouting(char* filename, int local_id, struct shared_mem *p_mem)
{
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

	if (result == FAILURE) {
		return FAILURE;
	}

	#ifdef DEBUG
	showTopology(topology);
	#endif
	// Dummy status_table
	NodeStatus status_table[topology.nodes_count];
	int i;
	for(i = 0; i < topology.nodes_count; i++){
		status_table[i] = ONLINE;
	}
	status_table[idToIndex(2)] = OFFLINE;
	// status_table[idToIndex(3)] = OFFLINE;
	showStatusTable(topology.nodes_count, status_table);

	RoutingTable routing_table = createRoutingTable(topology, bidir_connections, local_id, status_table);
	showRoutingTable(routing_table);

	outInit(p_mem, local_connections);
	sleep(50);
	//sayHello(local_connections);

	//pthread_t listen_thread;
	//pthread_create(&listen_thread, NULL, (void*) &routingListen, (void*) &local_port);
	//char c;
	/*while((c=getchar()) != EOF){
		sayHello(local_connections);
		//printf("still listening\n");
	}*/
	// routingListen(local_port);
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
	setAddressById(node_ID, &first_node, connections);
	routing_table.table[idToIndex(node_ID)] = first_node; 


	for(i=0; i<topology.neighbors_counts[idToIndex(node_ID)]; i++){			//adding close neighbors into stack and routing_table
		int new_node_ID  = indexToId(topology.table[idToIndex(node_ID)][i]);
		if(routing_table.table[idToIndex(new_node_ID)].next_hop_id == -1 &&
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

	while (visited_nodes <= topology.nodes_count && visited_nodes<last){	//untill there are some accesible unattended nodes
		int actual_node_ID = stack[visited_nodes].final_node_ID;
		#ifdef DEBUG
		printf("debug: visiting ID %d\n", actual_node_ID);
		#endif
		for(i=0; i<topology.neighbors_counts[idToIndex(actual_node_ID)]; i++){	//check actual node neighbors
			int new_node_ID  = indexToId(topology.table[idToIndex(actual_node_ID)][i]);
			if(routing_table.table[idToIndex(new_node_ID)].next_hop_id == -1 && status_table[idToIndex(new_node_ID)] == ONLINE){
				#ifdef DEBUG
				printf("debug: discovered it's neighbor %d\n", new_node_ID);
				#endif
				StackEntry new;
				RoutingTableEntry new_entry; //add IP and port!
				setAddressById(stack[visited_nodes].first_hop_node_ID, &new_entry, connections);
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

	routing_table.table[idToIndex(node_ID)].next_hop_id = -1;
	routing_table.table[idToIndex(node_ID)].next_hop_port = -1;
	strcpy(routing_table.table[idToIndex(node_ID)].next_hop_ip, "none");

	return routing_table;
}

int idToIndex(int id)
{
	return id - MIN_ID;
}

int indexToId(int index){
	return index + MIN_ID;
}

void showRoutingTable(RoutingTable routing_table)
{
	printf("---------------\n");
	printf("Routing table\n");
	printf("---------------\n");
	int i;
	for(i=0; i<routing_table.size; i++){
		if(routing_table.table[i].next_hop_id != -1){
			printf("route to %d via\t%d\t(address: %s:%d)\n", indexToId(i), 
					routing_table.table[i].next_hop_id, routing_table.table[i].next_hop_ip, routing_table.table[i].next_hop_port);
		}
	}
	printf("---------------\n\n");
}

void setAddressById(int id, RoutingTableEntry* entry, Connections connections)
{
	#ifdef DEBUG
	printf("debug setAddressById: looking for connection to %d\n", id);
	#endif
	int i;
	int state = FAILURE;
	for(i=0; i<connections.count; i++){
		if(connections.array[i].id == id){
			entry->next_hop_id = connections.array[i].id;
			entry->next_hop_port = connections.array[i].port;
			strcpy(entry->next_hop_ip, connections.array[i].ip_address);
			state = SUCCESS;
			break;
		}
	}
	if(state == FAILURE){
		#ifdef DEBUG
		printf("debug setAddressById: no connection found for id %d\n", id);
		#endif
		entry->next_hop_id = id;
		entry->next_hop_port = -1;
		strcpy(entry->next_hop_ip, "127.0.0.1");
	}
}

void showStatusTable(int status_table_size, NodeStatus* status_table)
{
	printf("-----------\n");
	printf("State table: \n");
	printf("-----------\n");
	int i;
	for(i=0; i<status_table_size; i++){
		if(status_table[i] == ONLINE){
			printf("node %d is ONLINE\n", indexToId(i));
		}else{
			printf("node %d is OFFLINE\n", indexToId(i));
		}
	}
	printf("-----------\n\n");
}
