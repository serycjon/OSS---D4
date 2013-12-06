#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dynamic_routing.h"
#include "packets.h"
#include "main.h"

char *formNSUPacket(int id, int new_state, int *len)
{
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
}

void parseMsg(struct mem_and_buffer_and_sfd *params)
{
	char *buf = params->buf;

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
}

void parseHello(struct mem_and_buffer_and_sfd *params)
{
	int len = params->len;
	char *buf = params->buf;

	if(len!=2*sizeof(char)){
		printf("INVALID hello!\n");
	}
	int source_id = (int)buf[1];
	struct real_connection *conn = &(params->mem->p_connections[source_id]);
	if(conn->online==OFFLINE){
		reactToStateChange(source_id, ONLINE, params->mem);
	}
	conn->type = IN_CONN;
	conn->id = source_id;
	conn->addr = params->addr;
	conn->sockfd = params->sfd;
	conn->last_seen = time(NULL);
	conn->online = ONLINE;

	//printf("HELLO from %d!\n", source_id);
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
	
	if(params->mem->p_status_table[id]!=new_state){
		printf("I have heard that node %d has changed its state!\n", id);
		reactToStateChange(id, new_state, params->mem);
	}
}

void parseDD(struct mem_and_buffer_and_sfd *params)
{
	printf("received DD\n");
}
