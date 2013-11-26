/*
 * File name: route_cfg_parser.h
 * Date:      2012/09/11 13:16
 * Author:    Jan Chudoba
 */

#ifndef __ROUTE_CFG_PARSER_H__
#define __ROUTE_CFG_PARSER_H__

#include <stdlib.h>

#define IP_ADDRESS_MAX_LENGTH 16

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
 *   fileName        - jmeno souboru, pokud je zadano NULL bere se vychozi nazev "routing.cfg"
 *   localId         - ID tohoto uzlu
 *   localPort       - pointer na int, kam se ulozi port lokalniho uzlu
 *                     (port ktery je treba otevrit)
 *   connectionCount - pointer na int, ve kterem je ulozena velikost pole 'connections'
 *                     po navratu z funkce je v promenne ulozen skutecny pocet nactenych spojeni
 *   connections     - pole, kam se ukladaji nactena spojeni
 *
 */

int parseRouteConfiguration(const char * fileName, int localId, int * localPort, int * connectionCount, TConnection * connections);

#endif

/* end of route_cfg_parser.h */
