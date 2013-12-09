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
	showTopology(*(p_mem->p_topology));
	#endif
	p_mem->p_status_table =(NodeStatus *) malloc((MAX_NODES+1)*sizeof(NodeStatus));
	int i;
	for(i = 0; i < MAX_NODES+1; i++){
		p_mem->p_status_table[i] = OFFLINE;
	}
	p_mem->p_status_table[local_id] = ONLINE;
	showStatusTable(p_mem);

	p_mem->p_routing_table = (RoutingTable *) calloc(1, sizeof(RoutingTable));
	createRoutingTable(p_mem);
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

	return SUCCESS;
}

void createRoutingTable (struct shared_mem *p_mem)
{
	Queue queue = (Queue) malloc((p_mem->p_topology->nodes_count) * sizeof(QueueEntry));
	//RoutingTable routing_table;
	pthread_mutex_lock(&(p_mem->mutexes->routing_mutex));
	p_mem->p_routing_table->table = (RoutingTableEntry*) calloc((p_mem->p_topology->nodes_count+1), sizeof(RoutingTableEntry));
	p_mem->p_routing_table->size = p_mem->p_topology->nodes_count+1;
	//pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));
	int visited_nodes, i, last;
	last = 0;
	visited_nodes = 0;

	//pthread_mutex_lock(&(p_mem->mutexes->routing_mutex));
	for(i=0; i<p_mem->p_topology->nodes_count+1;i++){
		//RoutingTableEntry no_path;
		//no_path.next_hop_port = -1;
		//no_path.next_hop_id = -1;
		//strcpy(no_path.next_hop_ip, "none");
		p_mem->p_routing_table->table[i].next_hop_id = NOT_VISITED;
	}
	//pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));

	// routing to myself probably won't be needed
	RoutingTableEntry first_node;		//add IP and port!
	//setAddressById(p_mem->locad_ID, &first_node, connections);
	first_node.next_hop_id = p_mem->local_id;
	//pthread_mutex_lock(&(p_mem->mutexes->routing_mutex));
	p_mem->p_routing_table->table[idToIndex(p_mem->local_id)] = first_node; 
	//pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));

	for(i=0; i<p_mem->p_topology->neighbors_counts[idToIndex(p_mem->local_id)]; i++){			//adding close neighbors into queue and routing_table
		int new_node_ID  = indexToId(p_mem->p_topology->table[idToIndex(p_mem->local_id)][i]);
		pthread_mutex_lock(&(p_mem->mutexes->status_mutex));		
		//pthread_mutex_lock(&(p_mem->mutexes->routing_mutex));		
		if(p_mem->p_status_table[idToIndex(new_node_ID)] == ONLINE &&
				p_mem->p_routing_table->table[idToIndex(new_node_ID)].next_hop_id == NOT_VISITED){
			QueueEntry new;
			RoutingTableEntry new_entry; //add IP and port!
			new_entry.next_hop_id = new_node_ID;
			new.final_node_ID=new_node_ID;
			new.first_hop_node_ID = new_node_ID;	
			queue[last++] = new;
			p_mem->p_routing_table->table[idToIndex(new_node_ID)] = new_entry;
		}
		//pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));
		pthread_mutex_unlock(&(p_mem->mutexes->status_mutex));
	}

	while (visited_nodes <= p_mem->p_topology->nodes_count && visited_nodes<last){	//untill there are some accesible unattended nodes
		int actual_node_ID = queue[visited_nodes].final_node_ID;
#ifdef DEBUG
		printf("debug: visiting ID %d\n", actual_node_ID);
#endif
		for(i=0; i<p_mem->p_topology->neighbors_counts[idToIndex(actual_node_ID)]; i++){	//check actual node neighbors
			int new_node_ID  = indexToId(p_mem->p_topology->table[idToIndex(actual_node_ID)][i]);
			pthread_mutex_lock(&(p_mem->mutexes->status_mutex));
			//pthread_mutex_lock(&(p_mem->mutexes->routing_mutex));	
			if(p_mem->p_routing_table->table[idToIndex(new_node_ID)].next_hop_id == NOT_VISITED && p_mem->p_status_table[idToIndex(new_node_ID)] == ONLINE){
#ifdef DEBUG
				printf("debug: discovered it's neighbor %d\n", new_node_ID);
#endif
				QueueEntry new;
				RoutingTableEntry new_entry; //add IP and port!
				new_entry.next_hop_id = queue[visited_nodes].first_hop_node_ID;
				new.final_node_ID=new_node_ID;
				new.first_hop_node_ID = queue[visited_nodes].first_hop_node_ID;
				queue[last++] = new;
				p_mem->p_routing_table->table[idToIndex(new_node_ID)] = new_entry;
			}	
			//pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));
			pthread_mutex_unlock(&(p_mem->mutexes->status_mutex));		
		}
		visited_nodes++;
	}

	free(queue);
	queue = NULL;

	//pthread_mutex_lock(&(p_mem->mutexes->routing_mutex));
	p_mem->p_routing_table->table[idToIndex(p_mem->local_id)].next_hop_id = NOT_VISITED;
	pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));

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
	pthread_mutex_lock(&(mem->mutexes->routing_mutex));
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
	pthread_mutex_unlock(&(mem->mutexes->routing_mutex));
	printf("---------------\n\n");
}

void showStatusTable(struct shared_mem *mem)
{
	printf("-----------\n");
	printf("State table: \n");
	printf("-----------\n");
	pthread_mutex_lock(&(mem->mutexes->status_mutex)); 
	int i;
	for(i=MIN_ID; i<mem->p_topology->nodes_count+1; i++){
		if(mem->p_status_table[i] == ONLINE){
			printf("node %d is ONLINE\n", indexToId(i));
		}else{
			printf("node %d is OFFLINE\n", indexToId(i));
		}
	}
	pthread_mutex_unlock(&(mem->mutexes->status_mutex));
	printf("-----------\n\n");
}
