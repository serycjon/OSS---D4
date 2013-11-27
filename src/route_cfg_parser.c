/*
 * File name: route_cfg_parser.c
 * Date:      2012/09/11 11:50
 * Author:    Jan Chudoba
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "route_cfg_parser.h"

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
				return 1;
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


int parseRouteConfiguration(char* file_name, int local_id, int* local_port, 
		int* local_connection_count, 	TConnection* local_connections,
		int* nodes_count, int* neighbors_counts, int** topology_table)
{
	int max_connections = *local_connection_count;
	*local_connection_count = 0;
	TConnection all_connections[max_connections];
	int all_connection_count = 0;

	int result;
	if (local_port) *local_port = -1;
	FILE * f = fopen(file_name, "r");
	if (f) {
		result = 1;
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
						result = 0;
					}
					if (parseWord(&l, tmp, tmp_size)) {
						c.port = atoi(tmp);
					} else c.port = 0;
					if (c.port == 0) {
						fprintf(stderr, "CFG_PARSER: [ERROR] invalid port '%s' on line '%s'\n", tmp, line);
						result = 0;
					}
					parseWord(&l, c.ip_address, IP_ADDRESS_MAX_LENGTH);
					//fprintf(stderr, "CFG_PARSER: [DEBUG] ID=%d, port=%d, IP=%s\n", c.id, c.port, c.ip_address);
					all_connections[all_connection_count++] = c;
					if (c.id == local_id && local_port != NULL) {
						*local_port = c.port;
					}
				} else if (strcmp(tmp, "link") == 0) {
					int id_client, id_server;
					if (parseWord(&l, tmp, tmp_size)) {
						id_client = atoi(tmp);
					} else id_client = 0;
					if (id_client == 0) {
						fprintf(stderr, "CFG_PARSER: [ERROR] invalid client ID '%s' on line '%s'\n", tmp, line);
						result = 0;
					}
					if (parseWord(&l, tmp, tmp_size)) {
						id_server = atoi(tmp);
					} else id_server = 0;
					if (id_server == 0) {
						fprintf(stderr, "CFG_PARSER: [ERROR] invalid server ID '%s' on line '%s'\n", tmp, line);
						result = 0;
					}
					//fprintf(stderr, "CFG_PARSER: [DEBUG] LINK %d -> %d\n", id_client, id_server);
					if (id_client == local_id) {
						int i;
						for (i=0; i<all_connection_count; i++) {
							if (all_connections[i].id == id_server) {
								local_connections[(*local_connection_count)++] = all_connections[i];
							}
						}
					}
				} else {
					fprintf(stderr, "CFG_PARSER: [ERROR] invalid configuration line '%s'\n", line);
					result = 0;
				}
			}
		}
		fclose(f);
	} else {
		fprintf(stderr, "CFG_PARSER: [ERROR] can not open file '%s'\n", file_name);
		result = 0;
	}
	//fprintf(stderr, "CFG_PARSER: [DEBUG] parsed %d nodes and %d own links\n", all_connection_count, *connection_count);
	return result;
}

/* end of route_cfg_parser.c */
