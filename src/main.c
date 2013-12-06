#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "routing_table.h"
#include "settings.h"
#include "packets.h"

void startInterface();

void print_usage(char* file_name){
	printf("Usage: %s <id> [ <config_file> ]\n", file_name);
	printf("Description: .............blablabla\n"
		"... blabla ...\n");
}

int main(int argc, char** argv){
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

	struct shared_mem mem;
	// Process the config file
	initRouting(config_file_name, node_id, &mem);
	startInterface(&mem);
	return 0;
}

void startInterface(struct shared_mem *mem)
{
	int dest_id;
	char buffer[250];
	char msg[250];
	int len;
	
	while(fgets(buffer, 250, stdin)){
	//	scanf("%d %s", &id, buffer)){
		//printf("%d %s\n", id, buffer);
		if(sscanf(buffer, "%d %s", &dest_id, msg) == 2){
			char *packet = formMsgPacket(mem->local_id, dest_id, msg, &len);
			sendToId(dest_id, packet, len, mem);
			//printf("dest_id: %d; msg: %s\n", id, msg);
		}else{
			printf("[ERROR] wrong format!\n"
					"please enter the destination ID and then your message.\n");
			//printf("You have entered: %s\n", buffer);
		}
	}
}
