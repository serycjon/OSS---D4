#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "dynamic_routing.h"
#include "packets.h"
#include "main.h"
#include "topology.h"
#include "buffer.h"

void printbincharpad(char c)
{
	int i;
	for (i = 7; i >= 0; --i)
	{
		putchar( (c & (1 << i)) ? '1' : '0' );
	}
	putchar('\n');
}

char *formDDRequestPacket(int source_id, int *len)
{
	char *msg = (char *) malloc(2*sizeof(char));
	msg[0] = T_DDR;
	msg[1] = (char) source_id;
	*len = 2; //strlen(msg+1)+sizeof(char);
	return msg;
}

char *formDDPacket(struct shared_mem *p_mem, int *len)
{
	unsigned char mask = 1 << 7; // only MSB is set
	unsigned char bit_field[32]; // we need 8*32 bits
	int i, bit_field_index, int_index;
	for(i=0; i < 32;   i++){
		bit_field[i] = 0; // better safe than sorry. probably isn't necessary
	}

	for(i=0; i < MAX_NODES+1; i++){
		pthread_mutex_lock(&(p_mem->mutexes->status_mutex));
		if(p_mem->p_status_table[i] == ONLINE){
			//printf("neco tu mame %d!\n", i);
			bit_field_index = i/8;
			int_index = i%8;
			bit_field[bit_field_index] |= mask >> int_index;
		}
		pthread_mutex_unlock(&(p_mem->mutexes->status_mutex));
	}
	// printf("bitfield: \n");
	// for(i=0; i<32; i++){
	// 	printf("%u, ", bit_field[i]);
	// }
	// printf("\n");

	char *msg = (char *) calloc(33, sizeof(char));
	msg[0] = T_DD;
	for(i=0; i<32; i++){
		int index = 1+i;
		*(msg+index) = bit_field[i];
	}
	//printf("will send: \n");
	//for(i=0; i<33;i++){
	//	printf("%d; ", (char) msg[i]);
	//}
	//printf("\nthat's it\n");
	//printbincharpad(msg[1]);
	*len = 8*4 + 1;
	return msg;
}

char *formNSUPacket(int id, int new_state, int *len)
{
	printf("sending NSU about node %d... went %d (online:%d)\n", id, new_state, ONLINE);
	char *msg = (char *) malloc(3*sizeof(char));
	msg[0] = T_NSU;
	msg[1] = (char) id;
	msg[2] = (char) new_state;
	// sprintf(msg+1, "Hi from %d!", id);
	*len = 3; //strlen(msg+1)+sizeof(char);
	return msg;
}

char *formHelloPacket(int id, int *len)
{
	char *msg = (char *) malloc(2*sizeof(char));
	msg[0] = T_HELLO;
	msg[1] = (u_char) id;
	// sprintf(msg+1, "Hi from %d!", id);
	*len = 2; //strlen(msg+1)+sizeof(char);
	return msg;
}

char *formMsgPacket(int source_id, int dest_id, char* text, int *len)
{
	char *msg = (char *) calloc(BUF_SIZE, sizeof(char));
	msg[0] = T_MSG;
	msg[1] = (u_char) dest_id;
	msg[2] = (u_char) source_id;
	strcpy(msg+3, text);
	// sprintf(msg+1, "Hi from %d!", id);
	*len = sizeof(char) * (3 + strlen(text) + 1); // (T_MSG, dest_id, source_id, msg, teminating \0)
	return msg;
}

void packetParser(void *parameter)
{
	struct mem_and_buffer_and_sfd *params = (struct mem_and_buffer_and_sfd *) parameter;
	//char *buf = params->buf;
	//printf("received: %s\n", buf);

	if(params->buf==NULL){
		printf("buffer NULL!!!!\n");
		return;
	}
	if(params->len < 1){
		printf("PACKET of zero length!!\n");
		//free(params->buf);
		//params->buf = NULL;
		return;
	}

	char type = params->buf[0];
	switch(type){
		case T_MSG:
			parseMsg(params);
			break;
		case T_HELLO:
			parseHello(params);
			break;
		case T_NSU:
			parseNSU(params);
			break;
		case T_DD:
			parseDD(params);
			break;
		case T_DDR:
			parseDDR(params);
			break;
		default:
			printf("INVALID TYPE!!! (%d)\n", type);
	}
	free(params->buf);
	params->buf = NULL;

	free(parameter);
}

void parseMsg(struct mem_and_buffer_and_sfd *params)
{
	if(params->len < 4 || params->buf==NULL){
		printf("packet too short\n");
		return;
	}

	int dest_id = (int)params->buf[1];
	int source_id = (int)params->buf[2];
#ifdef DEBUG
	//printf("Received MSG from %d to %d:\n"
			//"%s\n", source_id, dest_id, params->buf+3);
#endif
	if(dest_id!=params->mem->local_id){
#ifdef DEBUG
		printf("I should send it elsewhere!\n");
#endif
		sendToId(dest_id, params->buf, params->len, params->mem, RETRY);
	} else {
		printf("-----------\n");
		printf("Received message from node #%d:\n", source_id);
		printf("%s\n", params->buf+3);
		printf("-----------\n");
	}
	//free(params);
}

void parseHello(struct mem_and_buffer_and_sfd *params)
{
	int len = params->len;

	if(len!=2*sizeof(char)){
		printf("INVALID hello size!\n");
	}
	int source_id = (int)params->buf[1];
	//printf("Got hello from %d\n", source_id);
	pthread_mutex_lock(&(params->mem->mutexes->connection_mutex));
	struct real_connection *conn = &(params->mem->p_connections[source_id]);
	conn->type = IN_CONN;
	conn->id = source_id;
	conn->addr = params->addr; // deepcopy!
	conn->sockfd = params->sfd;
	conn->last_seen = time(NULL);
	conn->online = ONLINE;
	pthread_mutex_unlock(&(params->mem->mutexes->connection_mutex));

	reactToStateChange(source_id, ONLINE, params->mem);
}

void parseNSU(struct mem_and_buffer_and_sfd *params)
{
	//printf("received NSU\n");
	int len = params->len;

	if(len!=3*sizeof(char)){
		printf("INVALID NSU!\n");
	}
	int id = (int)params->buf[1];
	int new_state = (int)params->buf[2];

	pthread_mutex_lock(&(params->mem->mutexes->status_mutex));
	if(params->mem->p_status_table[id]!=new_state && !isNeighbour(params->mem->local_id, id, *(params->mem->p_topology))){
		printf("I have heard that node %d has changed its state!\n", id);
		pthread_mutex_unlock(&(params->mem->mutexes->status_mutex));
		reactToStateChange(id, new_state, params->mem);
	}else {
		pthread_mutex_unlock(&(params->mem->mutexes->status_mutex));
	}
	//free(params);
}

void parseDD(struct mem_and_buffer_and_sfd *params)
{
	unsigned char bit_field[32]; // we need 8*32 bits
	int len = params->len;
	int i, found, int_index;

	printf("received DD packet\n");
	for(i=0; i<32;i++){
		//printf("%u; ", (unsigned char) params->buf[i]);
		bit_field[i] = (unsigned char) params->buf[i+1]; // first byte is packet's TYPE
	}
	//printf("\nthat's it\n");
	if(len!=33*sizeof(char)){
		printf("INVALID DD length! (%d)\n", len);
	}
	int changed = 0;

	for(i=0; i < 32;   i++){
		for(int_index = 7; int_index >= 0; int_index--){
			if((bit_field[i]>>int_index & 1) == 1){
				//printf("???\n");
				found = i*8 +7- int_index;
				pthread_mutex_lock(&(params->mem->mutexes->status_mutex));
				if(params->mem->p_status_table[found] == OFFLINE){
					if(!isNeighbour(params->mem->local_id, found, *(params->mem->p_topology))){
					       //reactToStateChange(found, ONLINE, params->mem);
					        printf("according to DD %d is ONLINE\n", found);
					        changed++;
					        params->mem->p_status_table[found] = ONLINE;
						//pthread_mutex_unlock(&(params->mem->mutexes->status_mutex));
					       //sleep(1);
					        sendNSU(found, ONLINE, params->mem);
				       }else if(params->mem->p_connections[found].online == OFFLINE){
						//pthread_mutex_unlock(&(params->mem->mutexes->status_mutex));
	 					sendNSU(found, OFFLINE, params->mem);
				       }else{
						//pthread_mutex_unlock(&(params->mem->mutexes->status_mutex));
					}
				}else{
					//pthread_mutex_unlock(&(params->mem->mutexes->status_mutex));
				}
				pthread_mutex_unlock(&(params->mem->mutexes->status_mutex));
			}
		}
	}
	if(changed>0){
		createRoutingTable (params->mem);

		//pthread_mutex_lock(&(params->mem->mutexes->buffer_mutex));
#ifdef DEBUG
		showUndeliveredMessages(params->mem);
		printf("trying to resend them all\n");
#endif
		resendUndeliveredMessages(ALL, params->mem);
#ifdef DEBUG
		printf("trying to resend them all end\n");
#endif
		//pthread_mutex_unlock(&(params->mem->mutexes->buffer_mutex));
	}
}

void parseDDR(struct mem_and_buffer_and_sfd *params)
{
	int len = params->len;

	if(len!=2*sizeof(char)){
		printf("INVALID DDR length!\n");
	}
	int source_id = (int)params->buf[1];
	printf("received DDR from %d\n", source_id);

	int dd_len;
	char *dd = formDDPacket(params->mem, &dd_len);
	sendToNeighbour(source_id, dd, dd_len, params->mem);
	free(dd);
}
