#ifndef MAIN_GUARD
#define MAIN_GUARD
#include "settings.h"
#include "topology.h"
#include "routing_table.h"
#include "dynamic_routing.h"

typedef struct undelivered_message {
	int dest_id;
	int len;
	char* message;
	struct undelivered_message *next;
} UndeliveredMessage;


struct shared_mem {
	int local_id;
	int local_port;
	struct real_connection *p_connections;
	TopologyTable *p_topology;
	RoutingTable *p_routing_table;
	NodeStatus *p_status_table;
	struct all_mutexes *mutexes;
	UndeliveredMessage *buffer;
	int buf_size;
};

struct all_mutexes {
	pthread_mutex_t routing_mutex;
	pthread_mutex_t status_mutex;
	pthread_mutex_t connection_mutex;
	pthread_mutex_t buffer_mutex;
};


// needed for passing arguments to new threads
struct mem_and_sfd{
	int sfd;
	struct shared_mem *mem;
};

// needed for passing arguments to new parse threads
struct mem_and_buffer_and_sfd{
	int len;
	char *buf;
	int sfd;
	struct sockaddr *addr;
	struct shared_mem *mem;
};
#endif
