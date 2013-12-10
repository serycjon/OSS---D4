/*
 * File name: route_cfg_parser.c
 * Date:      2012/09/11 11:50
 * Author:    Jan Chudoba
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "route_cfg_parser.h"
#include "topology.h"
#include "settings.h"

#define DEFAULT_CFG_FILE_NAME "routing.cfg"
#define MAX_LINE_SIZE 255

int parseWord(char ** str, char * result, int max_length)
{
	int length = 0;
	char c;
	while ((c=**str) != 0) {
		if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
			if (length > 0) {
				result[length] = 0;
				return SUCCESS;
			}
		} else {
			if (length < max_length-1) {
				result[length++] = c;
			}
		}
		(*str)++;
	}
	result[length] = 0;
	return (length > 0);
}

void mallocCheck(void* pointer){
	if(pointer == NULL){
		printf("fatal error... malloc failed\n");
	}
}

int parseRouteConfiguration(char* file_name, int local_id, int* local_port, 
		pConnections p_local_connections, TopologyTable* p_topology)
{
	int all_connections_count = 0;
	TConnection all_connections[MAX_LINKS];


	p_topology->nodes_count = MAX_NODES;
	mallocCheck(p_topology->neighbors_counts = (int*) malloc(MAX_NODES * sizeof(int)));
	mallocCheck(p_topology->table = (int**) malloc(MAX_NODES * sizeof(int*)));
	p_topology->nodes_count = 0;

	//init topology table
	int i;
	for(i=0; i<MAX_NODES; i++){
		p_topology->neighbors_counts[i] = 0;
	}
	for(i=0; i<MAX_NODES; i++){
		mallocCheck(p_topology->table[i] = (int *) malloc(MAX_CONNECTIONS * sizeof(int)));
	}

	int* local_connection_count = &(p_local_connections->count);
	TConnection* local_connections = (p_local_connections->array);
	*local_connection_count = 0;

	int result;
	if (local_port) *local_port = -1;
	FILE * f = fopen(file_name, "r");
	if (f) {
		result = SUCCESS;
		char line[MAX_LINE_SIZE+1];
		while (!feof(f) && fgets(line, MAX_LINE_SIZE, f)) {
			int len = strlen(line);
			if (len > 0 && line[len-1] == '\n') { len--; line[len] = 0; }
			char * l = line;
			const int tmp_size = 20;
			char tmp[tmp_size];
			if (parseWord(&l, tmp, tmp_size)) {
				if (strcmp(tmp, "node") == 0) {
					TConnection c;
					if (parseWord(&l, tmp, tmp_size)) {
						c.id = atoi(tmp);
					} else c.id = 0;
					if (c.id == 0) {
						fprintf(stderr, "CFG_PARSER: [ERROR] invalid ID '%s' on line '%s'\n", tmp, line);
						result = FAILURE;
					}
					if (parseWord(&l, tmp, tmp_size)) {
						c.port = atoi(tmp);
					} else c.port = 0;
					if (c.port == 0) {
						fprintf(stderr, "CFG_PARSER: [ERROR] invalid port '%s' on line '%s'\n", tmp, line);
						result = FAILURE;
					}
					parseWord(&l, c.ip_address, IP_ADDRESS_MAX_LENGTH);
					if(strcmp(c.ip_address, "") == 0 || c.ip_address == NULL){
						strcpy(c.ip_address, "127.0.0.1");
					}
					//fprintf(stderr, "CFG_PARSER: [DEBUG] ID=%d, port=%d, IP=%s\n", c.id, c.port, c.ip_address);
					all_connections[all_connections_count++] = c;

					p_topology->nodes_count++;
					if(p_topology->nodes_count > MAX_NODES){
						fprintf(stderr, "CFG_PARSER: [ERROR] too many nodes!\n");
						return FAILURE;
					}
					if (c.id == local_id && local_port != NULL) {
						*local_port = c.port;
					}
					if(c.id > MAX_ID || c.id < MIN_ID){
						fprintf(stderr, "CFG_PARSER: [ERROR] id out of bounds!\n");
						return FAILURE;
					}
				} else if (strcmp(tmp, "link") == 0) {
					int id_client, id_server;
					if (parseWord(&l, tmp, tmp_size)) {
						id_client = atoi(tmp);
					} else id_client = 0;
					if (id_client == 0) {
						fprintf(stderr, "CFG_PARSER: [ERROR] invalid client ID '%s' on line '%s'\n", tmp, line);
						result = FAILURE;
					}
					if (parseWord(&l, tmp, tmp_size)) {
						id_server = atoi(tmp);
					} else id_server = 0;
					if (id_server == 0) {
						fprintf(stderr, "CFG_PARSER: [ERROR] invalid server ID '%s' on line '%s'\n", tmp, line);
						result = FAILURE;
					}
					//fprintf(stderr, "CFG_PARSER: [DEBUG] LINK %d -> %d\n", id_client, id_server);


					if (id_client == local_id) {
						int i;
						for (i=0; i<all_connections_count; i++) {
							if (all_connections[i].id == id_server) {
								local_connections[(*local_connection_count)++] = all_connections[i];
							}
						}
					}
					/* topology table construction */

					if(insertIntoTopologyTable(id_client, id_server, p_topology) == 0){
						result = FAILURE;
					}


				} else {
					fprintf(stderr, "CFG_PARSER: [ERROR] invalid configuration line '%s'\n", line);
					result = FAILURE;
				}
			}
		}
		
		fclose(f);
	} else {
		fprintf(stderr, "CFG_PARSER: [ERROR] can not open file '%s'\n", file_name);
		result = FAILURE;
	}
	//fprintf(stderr, "CFG_PARSER: [DEBUG] parsed %d nodes and %d own links\n", p_all_connections->count, *connection_count);
	return result;
}

void showConnections(Connections conns){
	printf("Connections: \n");
	int i;
	for(i=0; i<conns.count; i++){
		printf("connection to id: %d, on port %d and ip %s\n",
				conns.array[i].id,conns.array[i].port, conns.array[i].ip_address);
	}
	printf("-----------\n");
}
