#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#include "errors.h"
#include "network.h"
#include "route_cfg_parser.h"
#include "dynamic_routing.h"
#include "main.h"
#include "packets.h"
#include "topology.h"
#include "buffer.h"


void *get_in_addr(struct sockaddr *sa)
{
	if (sa == NULL) return "NULL";
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *get_in_port(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_port);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_port);
}

int inInit(void* mem)
{
	int port_number = ((struct shared_mem*)mem) ->local_port;
	DBG("inInit: Will listen on port: %d", port_number);

	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	ssize_t numbytes;
	struct sockaddr_storage their_addr;
	char buf[BUF_SIZE];
	socklen_t addr_len;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	char port[7];
	sprintf(port, "%d", port_number);

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	DBG("inInit: Listener waiting...\n");

	addr_len = sizeof(their_addr);

	for(;;){
		if ((numbytes = recvfrom(sockfd, buf, BUF_SIZE , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			continue;
		}
		
		// we have to deepcopy sockaddr so it doesnt get overwritten
		struct sockaddr *addr_cpy = (struct sockaddr *) malloc(sizeof(struct sockaddr));
		addr_cpy->sa_family = ((struct sockaddr *)&their_addr)->sa_family;
		memcpy(addr_cpy->sa_data, ((struct sockaddr *)&their_addr)->sa_data, sizeof(((struct sockaddr *)&their_addr)->sa_data));

		// same for the buffer
		char *buf_cpy = (char *) calloc((numbytes), sizeof(char));
		memcpy(buf_cpy, buf, (numbytes)*sizeof(char));

		struct mem_and_buffer_and_sfd *param = (struct mem_and_buffer_and_sfd *) calloc(1, sizeof(struct mem_and_buffer_and_sfd));
		param->buf = buf_cpy;
		param->len = numbytes;
		param->mem = (struct shared_mem *) mem;
		param->sfd = sockfd;
		param->addr = addr_cpy;
		
		pthread_t parse_thread;
		pthread_create(&parse_thread, NULL, (void*) &packetParser, (void*) param); 
		pthread_detach(parse_thread);
	}

	close(sockfd);

	return 0;
}

void outInit(struct shared_mem *mem, Connections out_conns)
{
	int i;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;   
	hints.ai_socktype = SOCK_DGRAM; 
	hints.ai_flags = 0;
	hints.ai_protocol = 0;      

	for(i=0; i<out_conns.count; i++){
		struct addrinfo *result, *rp;
		int sfd, s;
		char port_str[5];
		sprintf(port_str, "%d", out_conns.array[i].port);

		DBG("trying to connect to %s:%s\n", out_conns.array[i].ip_address, port_str);

		s = getaddrinfo(out_conns.array[i].ip_address, port_str, &hints, &result);
		if (s != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
			continue;
		}

		for (rp = result; rp != NULL; rp = rp->ai_next) {
			sfd = socket(rp->ai_family, rp->ai_socktype,
					rp->ai_protocol);
			if (sfd == -1){
				perror("outInitSocket");
				continue;
			}

			if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1){
				DBG("connected!");
				break;
			}
			perror("outInitConnect");

			close(sfd);
		}

		if (rp == NULL) {
			ERROR("Could not connect!!!");
			continue;
		}

		freeaddrinfo(result);

		pthread_mutex_lock(&(mem->mutexes->connection_mutex));
		struct mem_and_sfd *param;
		param = (struct mem_and_sfd*) malloc(sizeof(struct mem_and_sfd));
		param->sfd = sfd;
		param->mem = mem;
		pthread_mutex_unlock(&(mem->mutexes->connection_mutex));

		pthread_t listen_thread;
		pthread_create(&listen_thread, NULL, (void*) &sockListener, (void*) param);
		pthread_detach(listen_thread);

		// save the connection
		pthread_mutex_lock(&(mem->mutexes->connection_mutex));		
		struct real_connection *rc = &(mem->p_connections[out_conns.array[i].id]);
		rc->type = OUT_CONN;
		rc->id = out_conns.array[i].id;
		rc->sockfd = sfd;
		rc->last_seen = clock();
		rc->online = OFFLINE;
		pthread_mutex_unlock(&(mem->mutexes->connection_mutex));
	}
}

void sockListener(void *in_param)
{
	DBG("sockListener: starting new listener...");
	int sfd = ((struct mem_and_sfd*)in_param)->sfd;

	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len = sizeof(struct sockaddr_storage);
	ssize_t nread;
	char buf[BUF_SIZE];

	for (;;) {
		nread = recvfrom(sfd, buf, BUF_SIZE, 0,
				(struct sockaddr *) &peer_addr, &peer_addr_len);
		if (nread == -1){
			continue;
		}

		// copy buffer so that it doesn't get overwritten later
		char *buf_cpy = (char *) calloc((nread), sizeof(char));
		memcpy(buf_cpy, buf, (nread)*sizeof(char));

		struct mem_and_buffer_and_sfd *param = (struct mem_and_buffer_and_sfd *) calloc(1, sizeof(struct mem_and_buffer_and_sfd));
		param->buf = buf_cpy;
		param->len = nread;
		param->mem = ((struct mem_and_sfd *)in_param)->mem;
		param->sfd = sfd;
		param->addr = (struct sockaddr *)&peer_addr;

		pthread_t parse_thread;
		pthread_create(&parse_thread, NULL, (void*) &packetParser, (void*) param); 
		pthread_join(parse_thread, NULL);
	}
}

void sendToNeighbour(int dest_id, char *packet, int len, struct shared_mem *p_mem)
{
	pthread_mutex_lock(&(p_mem->mutexes->connection_mutex));
	struct real_connection *conns = p_mem->p_connections;
	// choose the right way to send according to connection direction
	if(conns[dest_id].type == OUT_CONN){
		sendto(conns[dest_id].sockfd, packet, len, 0, 0, 0);
	} else {
		socklen_t addr_len;
		addr_len = sizeof(*(conns[dest_id].addr));
		sendto(conns[dest_id].sockfd, packet, len, 0, conns[dest_id].addr, addr_len);
	}
	pthread_mutex_unlock(&(p_mem->mutexes->connection_mutex));
}
