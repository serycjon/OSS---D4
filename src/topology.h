#ifndef TOPOLOGY_GUARD
#define TOPOLOGY_GUARD


/* WARNING WARNING WARNING
 * there are INDICES inside the table, not IDs !!!
 * you have to convert them to IDs with indexToId(int index); from routing_table.h
 * WARNING WARNING WARNING
 */

typedef struct topology_table{
	int nodes_count;
	int* neighbors_counts;
	int** table;
} TopologyTable;


int insertIntoTopologyTable(int from, int to, TopologyTable* p_topology);
void showTopology(TopologyTable topology);
#endif
