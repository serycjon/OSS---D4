#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "network.h"
#include "errors.h"
#include "main.h"
#include "routing_table.h"
#include "settings.h"
#include "packets.h"
#include "buffer.h"

void startInterface();

void print_usage(char* file_name)
{
	printf("Usage: %s <id> [ <config_file> ]\n", file_name);
}

int main(int argc, char** argv)
{
	// Read parameters
	char* config_file_name;
	if(argc < 2 || argc > 3){
		print_usage(argv[0]);
		return 1;
	}else if(argc == 2){
		config_file_name = DEFAULT_CONFIG_FILENAME;
	}else{
		config_file_name = argv[2];
	}

	char *rest;
	int node_id = (int) strtol(argv[1], &rest, 10);

	if (*rest != '\0' || node_id < MIN_ID || node_id > MAX_ID) {
		fprintf(stderr, "ERROR: Invalid node id.\n");
		print_usage(argv[0]);
		return 1;
	}

	// allocate and init shared memory
	struct shared_mem *p_mem = (struct shared_mem *) calloc(1, sizeof(struct shared_mem));
	p_mem->mutexes = (struct all_mutexes *) malloc(sizeof(struct all_mutexes));
	pthread_mutex_init(&p_mem->mutexes->routing_mutex, NULL);
	pthread_mutex_init(&p_mem->mutexes->status_mutex, NULL);
	pthread_mutex_init(&p_mem->mutexes->connection_mutex, NULL);
	pthread_mutex_init(&p_mem->mutexes->buffer_mutex, NULL);

	initRouting(config_file_name, node_id, p_mem);

	startInterface(p_mem);
	return 0;
}

void startInterface(struct shared_mem *mem)
{
	int dest_id;
	char buffer[250];
	char msg[250];
	int len;
	
	while(fgets(buffer, 250, stdin)){
		if(sscanf(buffer, "%d %[^\n]s", &dest_id, msg) == 2){  // notice [^\n]... scanf can read even strings with spaces! :)
#ifdef DEBUG
			printf("dest_id: %d; msg: %s\n", dest_id, msg);
#endif
			if(strlen(msg)==0) continue;
			if(strlen(msg)>MAX_MSG_LEN){
				ERROR("message too long!");
			}
			char *packet = formMsgPacket(mem->local_id, dest_id, msg, &len);
			sendToId(dest_id, packet, len, mem, RETRY);
		}else if(strcmp("r\n", buffer) == 0){
			showRoutingTable(mem);
		}else if(strcmp("s\n", buffer) == 0){
			showStatusTable(mem);
		}else if(strcmp("b\n", buffer) == 0){
			showUndeliveredMessages(mem);
		}else{
			ERROR("wrong format!\n"
					"please enter the destination ID and then your message.");
		}
	}
}
