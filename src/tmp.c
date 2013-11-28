#include <stdio.h>
#include <stdlib.h>

//#define MAX_NODES 4

int searchNextNode(/*int ** topology_table, int* neighbours_counts, int nodes_count, */int first_node_ID, int final_node_ID/*, int* status_table*/){
	int* stack_for_nodes = (int *) malloc(MAX_NODES * 4 * sizeof(int));
	int* stack_for_parents = (int *) malloc(4 * sizeof(int));	
	int used_nodes [MAX_NODES+1];
	int number_of_used_nodes = 0;
	int number_of_actual_node = 0;
	int right_way, i;

	for (i=0; i<=MAX_NODES; i++){
		used_nodes[i] = 1;
	}

	for(i=0; i<neighbours_counts[first_node_ID-1]; i++){		//adds neighbours
		int new_node = topology_table[first_node_ID-1][i];
		if (status_table[new_node-1] == 0){				//0 == online?
			stack_for_nodes[number_of_used_nodes] = new_node;
			stack_for_parents[number_of_used_nodes] = new_node;
			number_of_used_nodes++;
			used_nodes[new_node-1] = 0;
		}
	}

	while(stack_for_nodes[number_of_actual_node] != final_node_ID){		//goes through distanted neighbours searching final node
		int actual_ID = stack_for_nodes[number_of_actual_node];
		
		for(i=0; i<neighbours_counts[actual_ID-1]; i++){			
			int new_node = topology_table[actual_ID-1][i];	
			if (status_table[new_node-1] == 0 && used_nodes[new_node-1] == 1){	//check if is online and haven't been used
				stack_for_nodes[number_of_used_nodes] = new_node;
				stack_for_parents[number_of_used_nodes] = stack_for_parents[number_of_actual_node];
				number_of_used_nodes++;
				used_nodes[new_node-1] = 0;
			}
		}
		number_of_actual_node++;
	}	

	right_way = stack_for_parents[number_of_actual_node];

	free(stack_for_nodes);
	free(stack_for_parents);
	stack_for_nodes = NULL;
	stack_for_parents = NULL;

	return right_way;
} 

int** getRoutingTable(){
	int** routing_table = (int **) malloc(MAX_NODES * sizeof(int *));
	int i, j;

	for(i=0; i<MAX_NODES; i++){
		for(j=0; j<MAX_NODES; j++){
//			routing_table[i][j] = searchNextNode(i+1, j+1);			
		}
	}
	return routing_table;
}

