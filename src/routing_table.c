#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "errors.h"
#include "network.h"
#include "route_cfg_parser.h"
#include "routing_table.h"
#include "topology.h"
#include "settings.h"
#include "dynamic_routing.h"
#include "main.h"

int initRouting(char* filename, int local_id, struct shared_mem *p_mem)
{
	// shared memory initialization
	int local_port;
	p_mem->p_topology = (TopologyTable *) malloc(sizeof(TopologyTable));

	TConnection local_conns[MAX_CONNECTIONS];
	Connections local_connections;
	local_connections.count = MAX_CONNECTIONS;
	local_connections.array =  local_conns;

	int result = parseRouteConfiguration(filename, local_id, &local_port, &local_connections, p_mem->p_topology);

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

	p_mem->p_connections = (struct real_connection*) malloc(MAX_NODES * sizeof(struct real_connection));
	p_mem->local_id = local_id;
	p_mem->local_port = local_port;

	for(i=0; i<MAX_NODES; i++){
		time_t tm = time(NULL);
		p_mem->p_connections[i].type = 0;
		p_mem->p_connections[i].id = -1;
		p_mem->p_connections[i].addr = NULL;
		p_mem->p_connections[i].sockfd = -1;
		p_mem->p_connections[i].last_seen = tm;
		p_mem->p_connections[i].online = OFFLINE;
	}

	// start doing the work
	outInit(p_mem, local_connections);

	pthread_t listen_thread;
	pthread_create(&listen_thread, NULL, (void*) &inInit, (void*) p_mem);

	pthread_t hello_sender;
	pthread_create(&hello_sender, NULL, (void*) &helloSender, (void*) p_mem);

	pthread_t death_checker_thread;
	pthread_create(&death_checker_thread, NULL, (void*) &deathChecker, (void*) p_mem);

	return SUCCESS;
}

void createRoutingTable (struct shared_mem *p_mem)
{
	Queue queue = (Queue) malloc((p_mem->p_topology->nodes_count) * sizeof(QueueEntry));

	pthread_mutex_lock(&(p_mem->mutexes->routing_mutex));
	pthread_mutex_lock(&(p_mem->mutexes->connection_mutex));
	if(p_mem->p_routing_table->table==NULL){
		p_mem->p_routing_table->table = (RoutingTableEntry*) calloc((p_mem->p_topology->nodes_count+1), sizeof(RoutingTableEntry));
		p_mem->p_routing_table->size = p_mem->p_topology->nodes_count+1;
	}

	int visited_nodes, i, last;
	last = 0;
	visited_nodes = 0;

	for(i=0; i<p_mem->p_topology->nodes_count+1;i++){
		p_mem->p_routing_table->table[i].next_hop_id = NOT_VISITED;
	}
	RoutingTableEntry first_node;
	first_node.next_hop_id = p_mem->local_id;
	p_mem->p_routing_table->table[idToIndex(p_mem->local_id)] = first_node; 
	
	//add close neighbors into queue and routing_table
	for(i=0; i<p_mem->p_topology->neighbors_counts[idToIndex(p_mem->local_id)]; i++){			
		int new_node_ID  = indexToId(p_mem->p_topology->table[idToIndex(p_mem->local_id)][i]);

		if(p_mem->p_status_table[idToIndex(new_node_ID)] == ONLINE &&
				p_mem->p_routing_table->table[idToIndex(new_node_ID)].next_hop_id == NOT_VISITED){
			QueueEntry new;
			RoutingTableEntry new_entry;			
			new_entry.next_hop_id = new_node_ID;
			new.final_node_ID=new_node_ID;
			new.first_hop_node_ID = new_node_ID;	
			queue[last++] = new;
			p_mem->p_routing_table->table[idToIndex(new_node_ID)] = new_entry;
		}
		pthread_mutex_unlock(&(p_mem->mutexes->status_mutex));
	}

	//while there are some accesible nodes not visited yet
	while (visited_nodes <= p_mem->p_topology->nodes_count && visited_nodes<last){
		int actual_node_ID = queue[visited_nodes].final_node_ID;

		DBG("createRoutingTable: visiting ID %d", actual_node_ID);

		for(i=0; i<p_mem->p_topology->neighbors_counts[idToIndex(actual_node_ID)]; i++){	//check actual node neighbors
			int new_node_ID  = indexToId(p_mem->p_topology->table[idToIndex(actual_node_ID)][i]);

			if(p_mem->p_routing_table->table[idToIndex(new_node_ID)].next_hop_id == NOT_VISITED && \
					p_mem->p_status_table[idToIndex(new_node_ID)] == ONLINE){

				DBG("createRoutingTable: discovered it's neighbor %d\n", new_node_ID);

				QueueEntry new;
				RoutingTableEntry new_entry;
				new_entry.next_hop_id = queue[visited_nodes].first_hop_node_ID;
				new.final_node_ID=new_node_ID;
				new.first_hop_node_ID = queue[visited_nodes].first_hop_node_ID;
				queue[last++] = new;
				p_mem->p_routing_table->table[idToIndex(new_node_ID)] = new_entry;
			}	
			pthread_mutex_unlock(&(p_mem->mutexes->status_mutex));		
		}
		visited_nodes++;
	}

	free(queue);
	queue = NULL;

	// set myself as unreachable
	p_mem->p_routing_table->table[idToIndex(p_mem->local_id)].next_hop_id = NOT_VISITED;
	pthread_mutex_unlock(&(p_mem->mutexes->connection_mutex));
	pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));

	DBG("createRoutingTable: routing table recomputed!\n");
}

int idToIndex(int id)
{
	return id; // - MIN_ID;
}

int indexToId(int index){
	return index; // + MIN_ID;
}

void showRoutingTable(struct shared_mem *mem)
{
	pthread_mutex_lock(&(mem->mutexes->routing_mutex));
	RoutingTable *routing_table = mem->p_routing_table;
	fprintf(stderr,"---------------\n");
	fprintf(stderr,"Routing table\n");
	fprintf(stderr,"---------------\n");
	int i;
	for(i=0; i<routing_table->size; i++){
		int id = routing_table->table[i].next_hop_id;
		if(id != -1){

			if(id == indexToId(i)){
				fprintf(stderr,"direct connection to %d\n", id);
			}else{
				fprintf(stderr,"route to %d via\t%d\n", indexToId(i), id);
			}
		}
	}
	pthread_mutex_unlock(&(mem->mutexes->routing_mutex));
	fprintf(stderr,"---------------\n\n");
}

void showStatusTable(struct shared_mem *mem)
{
	fprintf(stderr,"-----------\n");
	fprintf(stderr,"State table: \n");
	fprintf(stderr,"-----------\n");
	pthread_mutex_lock(&(mem->mutexes->status_mutex)); 
	int i;
	for(i=MIN_ID; i<mem->p_topology->nodes_count+1; i++){
		if(mem->p_status_table[i] == ONLINE){
			fprintf(stderr,"node %d is ONLINE\n", indexToId(i));
		}else{
			fprintf(stderr,"node %d is OFFLINE\n", indexToId(i));
		}
	}
	pthread_mutex_unlock(&(mem->mutexes->status_mutex));
	fprintf(stderr,"-----------\n\n");
}
