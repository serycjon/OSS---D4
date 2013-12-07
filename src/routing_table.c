#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "route_cfg_parser.h"
#include "routing_table.h"
#include "topology.h"
#include "settings.h"
#include "dynamic_routing.h"
#include "main.h"

int initRouting(char* filename, int local_id, struct shared_mem *p_mem)
{
	pthread_mutex_lock(&p_mem.mutexes.routing_mutex);
	pthread_mutex_lock(&p_mem.mutexes.status_mutex);
	pthread_mutex_lock(&p_mem.mutexes.connection_mutex);	
	int local_port;
	p_mem->p_topology = (TopologyTable *) malloc(sizeof(TopologyTable));

	TConnection local_conns[MAX_CONNECTIONS];
	Connections local_connections;
	local_connections.count = MAX_CONNECTIONS;
	local_connections.array =  local_conns;

	TConnection bidir_conns[MAX_CONNECTIONS];
	Connections bidir_connections;
	bidir_connections.count = MAX_CONNECTIONS;
	bidir_connections.array =  bidir_conns;

	int result = parseRouteConfiguration(filename, local_id, &local_port, &bidir_connections, &local_connections, p_mem->p_topology);

	if (result == FAILURE) {
		return FAILURE;
	}

	#ifdef DEBUG
	showTopology(topology);
	#endif
	p_mem->p_status_table =(NodeStatus *) malloc((MAX_NODES+1)*sizeof(NodeStatus));
	int i;
	for(i = 0; i < MAX_NODES+1; i++){
		p_mem->p_status_table[i] = OFFLINE;
	}
	p_mem->p_status_table[local_id] = ONLINE;
	showStatusTable(p_mem->p_topology->nodes_count, p_mem->p_status_table);

	p_mem->p_routing_table = (RoutingTable *) malloc(sizeof(RoutingTable));
	createRoutingTable(*(p_mem->p_topology), local_id, p_mem->p_status_table, p_mem->p_routing_table);
	//p_mem->p_routing_table = &routing_table;
	//showRoutingTable(p_mem);

	//printf("Topology table size: %d\n", p_mem->p_topology->nodes_count);
	//showRoutingTable(p_mem);

	//return SUCCESS;
	//shared mem filling

	p_mem->p_connections = (struct real_connection*) malloc(MAX_NODES * sizeof(struct real_connection));
	p_mem->local_id = local_id;
	p_mem->local_port = local_port;
	//p_mem->p_connections = real_conn_array;
	for(i=0; i<MAX_NODES; i++){
		time_t tm = time(NULL);
		p_mem->p_connections[i].type = 0;
		p_mem->p_connections[i].id = -1;
		p_mem->p_connections[i].addr = NULL;
		p_mem->p_connections[i].sockfd = -1;
		p_mem->p_connections[i].last_seen = tm;
		p_mem->p_connections[i].online = OFFLINE;
	}
	outInit(p_mem, local_connections);

	pthread_t listen_thread;
	pthread_create(&listen_thread, NULL, (void*) &inInit, (void*) p_mem);

	pthread_t hello_sender;
	pthread_create(&hello_sender, NULL, (void*) &helloSender, (void*) p_mem);

	pthread_t satan_kalous_thread;
	pthread_create(&satan_kalous_thread, NULL, (void*) &satanKalous, (void*) p_mem);
	
	pthread_mutex_unlock(&p_mem.mutexes.connection_mutex);
	pthread_mutex_unlock(&p_mem.mutexes.status_mutex);
	pthread_mutex_unlock(&p_mem.mutexes.routing_mutex);

	return SUCCESS;
}

void createRoutingTable (TopologyTable topology, int node_ID, NodeStatus* status_table, RoutingTable *routing_table)
{
	pthread_mutex_lock(&p_mem.mutexes.routing_mutex);
	pthread_mutex_lock(&p_mem.mutexes.status_mutex);
	Queue queue = (Queue) malloc((topology.nodes_count) * sizeof(QueueEntry));
	//RoutingTable routing_table;
	routing_table->table = (RoutingTableEntry*) malloc((topology.nodes_count+1) * sizeof(RoutingTableEntry));
	routing_table->size = topology.nodes_count;
	int visited_nodes, i, last;
	last = 0;
	visited_nodes = 0;


	for(i=0; i<topology.nodes_count+1;i++){
		//RoutingTableEntry no_path;
		//no_path.next_hop_port = -1;
		//no_path.next_hop_id = -1;
		//strcpy(no_path.next_hop_ip, "none");
		routing_table->table[i].next_hop_id = -1;
	}

	// routing to myself probably won't be needed
	RoutingTableEntry first_node;		//add IP and port!
	//setAddressById(node_ID, &first_node, connections);
	first_node.next_hop_id = node_ID;
	routing_table->table[idToIndex(node_ID)] = first_node; 


	for(i=0; i<topology.neighbors_counts[idToIndex(node_ID)]; i++){			//adding close neighbors into queue and routing_table
		int new_node_ID  = indexToId(topology.table[idToIndex(node_ID)][i]);
		if(status_table[idToIndex(new_node_ID)] == ONLINE&&
				routing_table->table[idToIndex(new_node_ID)].next_hop_id == -1){
			QueueEntry new;
			RoutingTableEntry new_entry; //add IP and port!
			new_entry.next_hop_id = new_node_ID;
			//setAddressById(new_node_ID, &new_entry, connections);
			new.final_node_ID=new_node_ID;
			new.first_hop_node_ID = new_node_ID;	
			queue[last++] = new;
			routing_table->table[idToIndex(new_node_ID)] = new_entry;
		}
	}

	while (visited_nodes <= topology.nodes_count && visited_nodes<last){	//untill there are some accesible unattended nodes
		int actual_node_ID = queue[visited_nodes].final_node_ID;
#ifdef DEBUG
		printf("debug: visiting ID %d\n", actual_node_ID);
#endif
		for(i=0; i<topology.neighbors_counts[idToIndex(actual_node_ID)]; i++){	//check actual node neighbors
			int new_node_ID  = indexToId(topology.table[idToIndex(actual_node_ID)][i]);
			if(routing_table->table[idToIndex(new_node_ID)].next_hop_id == -1 && status_table[idToIndex(new_node_ID)] == ONLINE){
#ifdef DEBUG
				printf("debug: discovered it's neighbor %d\n", new_node_ID);
#endif
				QueueEntry new;
				RoutingTableEntry new_entry; //add IP and port!
				new_entry.next_hop_id = queue[visited_nodes].first_hop_node_ID;
				//setAddressById(queue[visited_nodes].first_hop_node_ID, &new_entry, connections);
				new.final_node_ID=new_node_ID;
				new.first_hop_node_ID = queue[visited_nodes].first_hop_node_ID;
				queue[last++] = new;
				routing_table->table[idToIndex(new_node_ID)] = new_entry;
			}			
		}
		visited_nodes++;
	}

	free(queue);
	queue = NULL;

	routing_table->table[idToIndex(node_ID)].next_hop_id = -1;

	pthread_mutex_unlock(&p_mem.mutexes.status_mutex);
	pthread_mutex_unlock(&p_mem.mutexes.routing_mutex);
	//routing_table->table[idToIndex(node_ID)].next_hop_port = -1;
	//strcpy(routing_table->table[idToIndex(node_ID)].next_hop_ip, "none");

	//return routing_table;
}

int idToIndex(int id)
{
	return id; // - MIN_ID;
}

int indexToId(int index){
	return index;// + MIN_ID;
}

void showRoutingTable(struct shared_mem *mem)
{
	RoutingTable *routing_table = mem->p_routing_table;
	printf("---------------\n");
	printf("Routing table\n");
	printf("---------------\n");
	int i;
	for(i=0; i<routing_table->size; i++){
		int id = routing_table->table[i].next_hop_id;
		if(id != -1){

			if(id == indexToId(i)){
				printf("direct connection to %d\n", id);
			}else{
				printf("route to %d via\t%d\n", indexToId(i), id);
			}
		}
	}
	printf("---------------\n\n");
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
