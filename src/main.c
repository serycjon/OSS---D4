#include <stdio.h>
#include <stdlib.h>
#include "routing_table.h"
#include "settings.h"

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

	// Process the config file
	load_config_from_file(config_file_name, node_id);
	return 0;
}
