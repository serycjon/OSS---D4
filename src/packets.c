#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "dynamic_routing.h"
#include "packets.h"
#include "main.h"
#include "topology.h"

void printbincharpad(char c)
{
	int i;
	for (i = 7; i >= 0; --i)
	{
		putchar( (c & (1 << i)) ? '1' : '0' );
	}
	putchar('\n');
}

char *formDDPacket(NodeStatus *state_table, int *len)
{
	uint32_t mask = 1 << 31; // only MSB is set
	uint32_t bit_field[8]; // we need 8*32 bits
	int i, bit_field_index, int_index;
	for(i=0; i < 8;   i++){
		bit_field[i] = 0; // better safe than sorry. probably isn't necessary
	}

	for(i=0; i < MAX_NODES+1; i++){
		if(state_table[i] == ONLINE){
			//printf("neco tu mame!\n");
			bit_field_index = i/32;
			int_index = i%32;
			bit_field[bit_field_index] |= mask >> int_index;
		}
	}


	//printf("bits of first part %u\n", bit_field[0]);

	char *msg = (char *) calloc(33, sizeof(char));
	msg[0] = T_DD;
	for(i=0; i<8; i++){
		int index = 1+(i*4);
		//*(msg+index) = bit_field[i];
		*(msg+index) = htonl(bit_field[i]);
	}
	//printbincharpad(msg[1]);
	*len = 8*4 + 1;
	return msg;
}

char *formNSUPacket(int id, int new_state, int *len)
{
	//printf("sending NSU about node %d..\n", id);
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
	char *msg = (char *) malloc(BUF_SIZE*sizeof(char));
	msg[0] = T_MSG;
	msg[1] = (u_char) dest_id;
	msg[2] = (u_char) source_id;
	strcpy(msg+3, text);
	// sprintf(msg+1, "Hi from %d!", id);
	*len = 3*sizeof(char) + strlen(text); //strlen(msg+1)+sizeof(char);
	return msg;
}

void packetParser(void *parameter)
{
	struct mem_and_buffer_and_sfd *params = (struct mem_and_buffer_and_sfd *) parameter;
	char *buf = params->buf;
	//printf("received: %s\n", buf);

	if(params->len < 1 || buf==NULL){
		printf("PACKET of zero length!!\n");
		free(params->buf);
		params->buf = NULL;
		return;
	}

	char type = buf[0];
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
		default:
			printf("INVALID TYPE!!!\n");
	}
	free(params->buf);
	params->buf = NULL;
	//free(parameter);
}

void parseMsg(struct mem_and_buffer_and_sfd *params)
{
	char *buf = params->buf;
	if(params->len < 4 || params->buf==NULL){
		printf("packet too short\n");
		return;
	}

	int dest_id = (int)buf[1];
	int source_id = (int)buf[2];
#ifdef DEBUG
	printf("Received MSG from %d to %d:\n"
			"%s\n", source_id, dest_id, buf+3);
#endif
	if(dest_id!=params->mem->local_id){
#ifdef DEBUG
		printf("I should send it elsewhere!\n");
#endif
		sendToId(dest_id, buf, params->len, params->mem);
	} else {
		printf("Received message from node #%d:\n", source_id);
		printf("%s\n", buf+3);
	}
	//free(params);
}

void parseHello(struct mem_and_buffer_and_sfd *params)
{
	int len = params->len;
	char *buf = params->buf;

	if(len!=2*sizeof(char)){
		printf("INVALID hello size!\n");
	}
	int source_id = (int)buf[1];
	//printf("Got hello from %d\n", source_id);
	struct real_connection *conn = &(params->mem->p_connections[source_id]);
	conn->type = IN_CONN;
	conn->id = source_id;
	conn->addr = params->addr;
	conn->sockfd = params->sfd;
	conn->last_seen = time(NULL);
	conn->online = ONLINE;
	//if(conn->online==OFFLINE){
	reactToStateChange(source_id, ONLINE, params->mem);
	//}

	//printf("HELLO from %d!\n", source_id);
	//free(params);
}

void parseNSU(struct mem_and_buffer_and_sfd *params)
{
	//printf("received NSU\n");
	int len = params->len;
	char *buf = params->buf;

	if(len!=3*sizeof(char)){
		printf("INVALID NSU!\n");
	}
	int id = (int)buf[1];
	int new_state = (int)buf[2];

	if(params->mem->p_status_table[id]!=new_state && !isNeighbour(params->mem->local_id, id, *(params->mem->p_topology))){
		printf("I have heard that node %d has changed its state!\n", id);
		reactToStateChange(id, new_state, params->mem);
	}
	//free(params);
}

void parseDD(struct mem_and_buffer_and_sfd *params)
{
	uint32_t bit_field[8]; // we need 8*32 bits
	int len = params->len;
	char *buf = params->buf;

	printf("received DD packet\n");
	if(len!=33*sizeof(char)){
		printf("INVALID DD length! (%d)\n", len);
	}
	int i, found, int_index;
	int changed = 0;
	for(i=0; i < 8;   i++){
		bit_field[i] = ntohl(*(buf+1+i*4));

		for(int_index = 31; int_index > 0; int_index--){
			if((bit_field[i]>>int_index & 1) == 1){
				found = i*32 +31- int_index;
				if(params->mem->p_status_table[found] == OFFLINE){
				       if(!isNeighbour(params->mem->local_id, found, *(params->mem->p_topology))){
					       //reactToStateChange(found, ONLINE, params->mem);
					       printf("according to DD %d is ONLINE\n", found);
					       changed++;
					       params->mem->p_status_table[found] = ONLINE;
					       //sleep(1);
					       sendNSU(found, ONLINE, params->mem);
				       }else if(params->mem->p_connections[found].online == OFFLINE){
					       sendNSU(found, OFFLINE, params->mem);
				       }
				}
			}
		}
	}
	if(changed>0){
		RoutingTable *new_routing_table = (RoutingTable *) malloc(sizeof(RoutingTable));
		createRoutingTable (*(params->mem->p_topology), params->mem->local_id, params->mem->p_status_table, new_routing_table);
		//RoutingTable *old_routing_table = mem->p_routing_table;
		params->mem->p_routing_table = new_routing_table;
	}
}
