#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#include "errors.h"
#include "network.h"
#include "route_cfg_parser.h"
#include "dynamic_routing.h"
#include "main.h"
#include "packets.h"
#include "topology.h"
#include "buffer.h"


void helloSender(void *param)
{
	struct shared_mem *mem = (struct shared_mem*) param;
	int len;
	char *msg = formHelloPacket(mem->local_id, &len);

	for(;;){
		// 0 ~ send really to all neighbours
		DBG("helloSender: saying hello!");
		sendToNeighbours(0, msg, len, mem);
		sleep(HELLO_TIMER);
	}
}

void ddrSender(void *param)
{
	struct shared_mem *mem = (struct shared_mem*) param;
	int len;
	char *msg = formDDRequestPacket(mem->local_id, &len);

	for(;;){
		sleep(DDR_TIMER);
		// 0 ~ send really to all neighbours
		DBG("helloSender: saying hello!");
		sendToNeighbours(0, msg, len, mem);
	}
}

void deathChecker(void *param)
{
	struct shared_mem *mem = ((struct shared_mem*) param);
	struct real_connection *conns = mem->p_connections;

	time_t end;
	double time_since_last_seen;


	int i;
	for(;;){
		DBG("deathChecker: Let's check all the nodes!");

		end = time(NULL);
		for(i=MIN_ID; i<MAX_NODES; i++){
			if(i==mem->local_id) continue;

			pthread_mutex_lock(&(mem->mutexes->connection_mutex));
			if(conns[i].online==ONLINE){
				time_since_last_seen = difftime(end, conns[i].last_seen);
				pthread_mutex_unlock(&(mem->mutexes->connection_mutex));

				if(time_since_last_seen > DEATH_TIMER){
					DBG("deathChecker: node %d is dead!", i);
					pthread_mutex_lock(&(mem->mutexes->connection_mutex));
					conns[i].online = OFFLINE;
					pthread_mutex_unlock(&(mem->mutexes->connection_mutex));
					reactToStateChange(i, OFFLINE, mem);
				}
				DBG("deathChecker: node %d is OK!", i);
				
			}else{
				pthread_mutex_unlock(&(mem->mutexes->connection_mutex));
			}
		}
		sleep(DEATH_TIMER);
	}
}

void reactToStateChange(int id, int new_state, struct shared_mem *mem)
{
	pthread_mutex_lock(&(mem->mutexes->status_mutex));
	// check if anything happened at all
	if(mem->p_status_table[id]==new_state) {
		pthread_mutex_unlock(&(mem->mutexes->status_mutex));
		return;
	}else {
		pthread_mutex_unlock(&(mem->mutexes->status_mutex));
	}

	if(new_state == ONLINE){
		fprintf(stderr,"NODE %d WENT ONLINE!\n", id);
	}else{
		fprintf(stderr,"NODE %d WENT OFFLINE!\n", id);
	}
	pthread_mutex_lock(&(mem->mutexes->status_mutex));
	mem->p_status_table[id] = new_state;
	pthread_mutex_unlock(&(mem->mutexes->status_mutex));

#ifdef DEBUG
	showStatusTable(mem);
#endif

	createRoutingTable (mem);

	// tell others too
	sendNSU(id, new_state, mem);

	// mark newly unreachable nodes as offline
	if(new_state == OFFLINE){
		int i;
		pthread_mutex_lock(&(mem->mutexes->routing_mutex));
		pthread_mutex_lock(&(mem->mutexes->status_mutex));				
		for(i=0; i<mem->p_routing_table->size; i++){
			if(i!=mem->local_id && mem->p_routing_table->table[i].next_hop_id == -1){
				mem->p_status_table[i] = OFFLINE;
			}
		}
		pthread_mutex_unlock(&(mem->mutexes->status_mutex));
		pthread_mutex_unlock(&(mem->mutexes->routing_mutex));
	}

	if(new_state == ONLINE && isNeighbour(mem->local_id, id, *(mem->p_topology))){
		// give some time to connection estabilishing
		sleep(1);
		int len;
		char *packet = formDDRequestPacket(mem->local_id, &len);
		sendToNeighbour(id, packet, len, mem);
	}

	resendUndeliveredMessages(id, mem);
#ifdef DEBUG
	DBG("ROUTING TABLE UPDATED!!!");
	showRoutingTable(mem);
#endif
}

void sendNSU(int id, int new_state, struct shared_mem *mem)
{
	int len;
	char *packet = formNSUPacket(id, new_state, &len);
	sendToNeighbours(id, packet, len, mem);
}

void sendToNeighbours(int not_to, char *packet, int len, struct shared_mem *p_mem)
{
	int id;
	pthread_mutex_lock(&(p_mem->mutexes->connection_mutex));
	struct real_connection *conns = p_mem->p_connections;
	for(id=0; id<MAX_NODES; id++){
		if(id != not_to && conns[id].id!=-1){
			pthread_mutex_unlock(&(p_mem->mutexes->connection_mutex));			
			sendToNeighbour(id, packet, len, p_mem);
			pthread_mutex_lock(&(p_mem->mutexes->connection_mutex));	
		}
	}
	pthread_mutex_unlock(&(p_mem->mutexes->connection_mutex));
}


int sendToId(int dest_id, char *packet, int len, struct shared_mem *p_mem, int retry)
{
	pthread_mutex_lock(&(p_mem->mutexes->routing_mutex));
	if(dest_id > p_mem->p_routing_table->size){
		ERROR("cannot reach node %d -- ID too big!", dest_id);
		pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));
		return -1;
	}else{
		pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));
	}
	if(dest_id == p_mem->local_id){
		ERROR("why would you send anything to yourself!?!");
		return -1;
	}
	pthread_mutex_lock(&(p_mem->mutexes->routing_mutex));

	DBG("trying to reach id %d\n", dest_id);
	int next_id = p_mem->p_routing_table->table[idToIndex(dest_id)].next_hop_id;
	if(next_id==-1){
		fprintf(stderr,"cannot reach node %d. %s\n", dest_id, retry==RETRY?"Will try it later":"");
		pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));

		if(retry==RETRY){
			addWaitingMessage(dest_id, len, packet, p_mem);;
		}
		return -1;
	}else{
		pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));
	}
	sendToNeighbour(next_id, packet, len, p_mem);
	return 0;
}
