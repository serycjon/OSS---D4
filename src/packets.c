#include <stdlib.h>
#include <stdio.h>
#include "packets.h"
#include "main.h"

char *formHelloPacket(int id)
{
	char *msg = (char *) malloc(20*sizeof(char));
	sprintf(msg, "Hi from %d!", id);
	return msg;
}

void packetParser(void *parameter)
{
	struct mem_and_buffer *params = (struct mem_and_buffer *) parameter;
	printf("received: %s\n", params->buf);

}
