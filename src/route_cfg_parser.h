/*
 * File name: route_cfg_parser.h
 * Date:      2012/09/11 13:16
 * Original author:    Jan Chudoba
 */
#ifndef __ROUTE_CFG_PARSER_H__
#define __ROUTE_CFG_PARSER_H__

#include <stdlib.h>
#include "settings.h"
#include "topology.h"

typedef struct {
	int id;
	int port;
	char ip_address[IP_ADDRESS_MAX_LENGTH];
} TConnection;

typedef struct local_connections{
	int count;
	TConnection* array;
} Connections, *pConnections;


int parseRouteConfiguration(char* file_name, int local_id, int* local_port, pConnections p_local_connections, TopologyTable* p_topology);
void showConnections(Connections conns);
#endif

/* end of route_cfg_parser.h */
