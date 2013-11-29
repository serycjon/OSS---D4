#ifndef TOPOLOGY_GUARD
#define TOPOLOGY_GUARD

typedef struct topology_table{
	int nodes_count;
	int* neighbors_counts;
	int** table;
} TopologyTable;

int insertIntoTopologyTable(int from, int to, TopologyTable* p_topology);
void showTopology(TopologyTable topology);
#endif
