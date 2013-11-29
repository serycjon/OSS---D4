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


/**
 * parseRouteConfiguration
 *
 * Nacteni konfiguracniho souboru s tabulkou propojeni mezi uzly
 *
 *   local_id         				- ID of this node
 *   local_port       				- pointer to int, in which we store this node's port (we will need to open it)
 *   local_connection_count		- pointer to int, in which the size of local_connections is stored 
 *   														the real number of nodes loaded is stored there after return
 *   local_connections   		  - array, where connections directly from this node are stored
 *
 *   global_connection_count		- pointer to int, in which the size of gloabl_connections is stored 
 *   														the real number of nodes loaded is stored there after return
 *   global_connections   		  - array, where all connections in the network are stored
 *
 *
 */

int parseRouteConfiguration(char* file_name, int local_id, int* local_port, 
		int* local_connection_count, 	TConnection* local_connections, TopologyTable* p_topology);
#endif

/* end of route_cfg_parser.h */
