#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "packets.h"
#include "main.h"

char *formHelloPacket(int id, int *len)
{
	char *msg = (char *) malloc(2*sizeof(char));
	msg[0] = T_HELLO;
	msg[1] = (u_char) id;
	// sprintf(msg+1, "Hi from %d!", id);
	*len = 2; //strlen(msg+1)+sizeof(char);
	return msg;
}

void packetParser(void *parameter)
{
	struct mem_and_buffer *params = (struct mem_and_buffer *) parameter;
	char *buf = params->buf;
	//printf("received: %s\n", buf);

	char type = buf[0];
	switch(type){
		case T_MSG:
			printf("MESG!\n");
			break;
		case T_HELLO:
			parseHello(params);
			break;
		case T_NSU:
			printf("NSU!\n");
			break;
		case T_DD:
			printf("DD!\n");
			break;
		default:
			printf("INVALID TYPE!!!\n");
	}


}

void parseMsg(struct mem_and_buffer *params)
{
	char *buf = params->buf;

	int dest_id = (int)buf[1];
	int source_id = (int)buf[2];
	printf("Received MSG from %d to %d:\n"
			"%s\n", source_id, dest_id, buf+3);
}

void parseHello(struct mem_and_buffer *params)
{
	int len = params->len;
	char *buf = params->buf;

	if(len!=2*sizeof(char)){
		printf("INVALID hello!\n");
	}
	int source_id = (int)buf[1];
	printf("HELLO from %d!\n", source_id);
}

void parseNSU(struct mem_and_buffer *params)
{
	printf("received NSU\n");
}

void parseDD(struct mem_and_buffer *params)
{
	printf("received DD\n");
}
