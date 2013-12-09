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

#include "route_cfg_parser.h"
#include "dynamic_routing.h"
#include "main.h"
#include "packets.h"
#include "topology.h"
#include "buffer.h"

#define MAXBUFLEN 100

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
	// printf("port: %d\n", port_number);
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	ssize_t numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	//char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
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

	//printf("listener: waiting to recvfrom...\n");

	addr_len = sizeof their_addr;
	for(;;){
		if ((numbytes = recvfrom(sockfd, buf, BUF_SIZE , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			continue;
		}

		//printf("numbytes: %zd\n", numbytes);

		// we have to deepcopy sockaddr so it doesnt get overwritten
		//
		// FREE IT!
		struct sockaddr *addr_cpy = (struct sockaddr *) malloc(sizeof(struct sockaddr));
		addr_cpy->sa_family = ((struct sockaddr *)&their_addr)->sa_family;
		memcpy(addr_cpy->sa_data, ((struct sockaddr *)&their_addr)->sa_data, sizeof(((struct sockaddr *)&their_addr)->sa_data));

		char *buf_cpy = (char *) calloc((numbytes), sizeof(char));
		memcpy(buf_cpy, buf, (numbytes)*sizeof(char));

		struct mem_and_buffer_and_sfd *param = (struct mem_and_buffer_and_sfd *) calloc(1, sizeof(struct mem_and_buffer_and_sfd));
		// remove???
		pthread_mutex_init(&param->mutex, NULL);
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
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */


	for(i=0; i<out_conns.count; i++){
		struct addrinfo *result, *rp;
		int sfd, s;
		char port_str[5];
		sprintf(port_str, "%d", out_conns.array[i].port);
		/* Obtain address(es) matching host/port */
		//printf("trying to connect to %s:%s\n", out_conns.array[i].ip_address, port_str);
		s = getaddrinfo(out_conns.array[i].ip_address, port_str, &hints, &result);
		if (s != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
			//exit(EXIT_FAILURE);
			continue;
		}

		/* getaddrinfo() returns a list of address structures.
		   Try each address until we successfully connect(2).
		   If socket(2) (or connect(2)) fails, we (close the socket
		   and) try the next address. */

		for (rp = result; rp != NULL; rp = rp->ai_next) {
			sfd = socket(rp->ai_family, rp->ai_socktype,
					rp->ai_protocol);
			if (sfd == -1){
				perror("outInitSocket");
				continue;
			}

			if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1){
				// printf("connected!!!\n");
				break;                  /* Success */
			}
			perror("outInitConnect");

			close(sfd);
		}

		if (rp == NULL) {               /* No address succeeded */
			fprintf(stderr, "Could not connect!!!\n");
			//exit(EXIT_FAILURE);
			continue;
		}

		//printf("connection succesfull\n");

		freeaddrinfo(result);           /* No longer needed */

		/* Send remaining command-line arguments as separate
		   datagrams, and read responses from server */
		//printf("created socket: %d\n", sfd);

		pthread_mutex_lock(&(mem->mutexes->connection_mutex));
		struct mem_and_sfd *param;
		param = (struct mem_and_sfd*) malloc(sizeof(struct mem_and_sfd));
		param->sfd = sfd;
		param->mem = mem;
		pthread_mutex_unlock(&(mem->mutexes->connection_mutex));

		pthread_t listen_thread;
		pthread_create(&listen_thread, NULL, (void*) &sockListener, (void*) param);
		pthread_detach(listen_thread);

		//sendto(sfd, "Ahojky!", strlen("Ahojky!")+1, 0, 0, 0);

		// printf("debug: %d\n", out_conns.array[i].id);
		// printf("dalsi: %d\n", mem->p_routing_table->table[4].next_hop_id);

		pthread_mutex_lock(&(mem->mutexes->connection_mutex));		
		struct real_connection *rc = &(mem->p_connections[out_conns.array[i].id]);
		rc->type = OUT_CONN; // OUT connection
		rc->id = out_conns.array[i].id;
		rc->sockfd = sfd;
		rc->last_seen = clock();
		rc->online = OFFLINE;
		pthread_mutex_unlock(&(mem->mutexes->connection_mutex));
	}
}

void sockListener(void *in_param)
{
	//printf("starting new listener...\n");
	int sfd = ((struct mem_and_sfd*)in_param)->sfd;
	//printf("using listening socket: %d\n", sfd);
	//int s;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len = sizeof(struct sockaddr_storage);
	ssize_t nread;
	char buf[BUF_SIZE];

	for (;;) {
		nread = recvfrom(sfd, buf, BUF_SIZE, 0,
				(struct sockaddr *) &peer_addr, &peer_addr_len);
		if (nread == -1){
			//printf("request failed\n");
			//perror("recv:");
			continue;               /* Ignore failed request */
		}

		char *buf_cpy = (char *) calloc((nread), sizeof(char));
		memcpy(buf_cpy, buf, (nread)*sizeof(char));
		//printf("size: %zd\n", nread);
		struct mem_and_buffer_and_sfd *param = (struct mem_and_buffer_and_sfd *) calloc(1, sizeof(struct mem_and_buffer_and_sfd));
		param->buf = buf_cpy;
		param->len = nread;
		param->mem = ((struct mem_and_sfd *)in_param)->mem;
		param->sfd = sfd;
		param->addr = (struct sockaddr *)&peer_addr;

		pthread_t parse_thread;
		pthread_create(&parse_thread, NULL, (void*) &packetParser, (void*) param); 
		pthread_join(parse_thread, NULL);

		//free(buf_cpy);
	}
}

void helloSender(void *param)
{
	struct shared_mem *mem = (struct shared_mem*) param;
	//struct real_connection *conns = mem.p_connections;
	int len;
	char *msg = formHelloPacket(mem->local_id, &len);

	//int i;
	for(;;){
		// 0 ~ send really to all neighbours
		//printf("saying hello!\n");
		sendToNeighbours(0, msg, len, mem);
		sleep(HELLO_TIMER);
	}
}

void satanKalous(void *param)
{
	struct shared_mem *mem = ((struct shared_mem*) param);
	struct real_connection *conns = mem->p_connections;

	time_t end;
	double time_since_last_seen;


	int i;
	for(;;){
		//printf("SATAN KALOUS says: Let's check all the nodes!\n");

		end = time(NULL);
		for(i=MIN_ID; i<MAX_NODES; i++){
			if(i==mem->local_id) continue;
			pthread_mutex_lock(&(mem->mutexes->connection_mutex));
			if(conns[i].online==ONLINE){
				time_since_last_seen = difftime(end, conns[i].last_seen);
				pthread_mutex_unlock(&(mem->mutexes->connection_mutex));
				//printf("node %d last seen %lf s ago...\n", conns[i].id, time_since_last_seen);
				if(time_since_last_seen > DEATH_TIMER){
					printf("SATAN KALOUS says: node %d is dead!\n", i);
					pthread_mutex_lock(&(mem->mutexes->connection_mutex));
					conns[i].online = OFFLINE;
					pthread_mutex_unlock(&(mem->mutexes->connection_mutex));
					reactToStateChange(i, OFFLINE, mem);
					//printf("NODE %d went OFFLINE!!!\n", conns[i].id);
				}
				//printf("SATAN KALOUS says: node %d is OK!\n", i);
				
			}else{
				pthread_mutex_unlock(&(mem->mutexes->connection_mutex));
			}
		}
		sleep(DEATH_TIMER);
	}
}

void reactToStateChange(int id, int new_state, struct shared_mem *mem)
{
	pthread_mutex_lock(&(mem->mutexes->status_mutex));
	if(mem->p_status_table[id]==new_state) {
		pthread_mutex_unlock(&(mem->mutexes->status_mutex));
		return;
	}else {
		pthread_mutex_unlock(&(mem->mutexes->status_mutex));
	}

	if(new_state == ONLINE){
		printf("NODE %d WENT ONLINE!\n", id);
	}else{
		printf("NODE %d WENT OFFLINE!\n", id);
	}
	pthread_mutex_lock(&(mem->mutexes->status_mutex));
	mem->p_status_table[id] = new_state;
	pthread_mutex_unlock(&(mem->mutexes->status_mutex));

#ifdef DEBUG
	showStatusTable(mem);
#endif

	createRoutingTable (mem);



	sendNSU(id, new_state, mem);

	printf("NSU sent! :D\n");
	if(new_state == OFFLINE){
		int i;
		pthread_mutex_lock(&(mem->mutexes->routing_mutex));
		pthread_mutex_lock(&(mem->mutexes->status_mutex));				
		for(i=0; i<mem->p_routing_table->size; i++){
			if(i!=mem->local_id && mem->p_routing_table->table[i].next_hop_id == -1){
				mem->p_status_table[i] = OFFLINE;
			}
		}
		pthread_mutex_unlock(&(mem->mutexes->status_mutex));
		pthread_mutex_unlock(&(mem->mutexes->routing_mutex));
	}
	sleep(1);
	if(new_state == ONLINE && isNeighbour(mem->local_id, id, *(mem->p_topology))){
		int len;
		char *packet = formDDRequestPacket(mem->local_id, &len);
		sendToNeighbour(id, packet, len, mem);
	}

	//pthread_mutex_lock(&(mem->mutexes->buffer_mutex));
	//sleep(2);
	resendUndeliveredMessages(id, mem);
	//pthread_mutex_unlock(&(mem->mutexes->buffer_mutex));
	/* FREE AS A BIRD!!! */
	//free(old_routing_table);
#ifdef DEBUG
	printf("ROUTING TABLE UPDATED!!!\n");
	showRoutingTable(mem);
#endif
}

void sendNSU(int id, int new_state, struct shared_mem *mem)
{
	int len;
	char *packet = formNSUPacket(id, new_state, &len);
	sendToNeighbours(id, packet, len, mem);
}

void sendToNeighbours(int not_to, char *packet, int len, struct shared_mem *p_mem)
{
	int id;
	pthread_mutex_lock(&(p_mem->mutexes->connection_mutex));
	struct real_connection *conns = p_mem->p_connections;
	for(id=0; id<MAX_NODES; id++){
		if(id != not_to && conns[id].id!=-1){
			pthread_mutex_unlock(&(p_mem->mutexes->connection_mutex));			
			sendToNeighbour(id, packet, len, p_mem);
			pthread_mutex_lock(&(p_mem->mutexes->connection_mutex));	
		}
	}
	pthread_mutex_unlock(&(p_mem->mutexes->connection_mutex));
}

void sendToNeighbour(int dest_id, char *packet, int len, struct shared_mem *p_mem)
{
	pthread_mutex_lock(&(p_mem->mutexes->connection_mutex));
	struct real_connection *conns = p_mem->p_connections;
	if(conns[dest_id].type == OUT_CONN){
		sendto(conns[dest_id].sockfd, packet, len, 0, 0, 0);
	} else {
		socklen_t addr_len;
		addr_len = sizeof(*(conns[dest_id].addr));
		sendto(conns[dest_id].sockfd, packet, len, 0, conns[dest_id].addr, addr_len);
	}
	pthread_mutex_unlock(&(p_mem->mutexes->connection_mutex));
}

int sendToId(int dest_id, char *packet, int len, struct shared_mem *p_mem, int retry)
{
	pthread_mutex_lock(&(p_mem->mutexes->routing_mutex));
	if(dest_id > p_mem->p_routing_table->size){
		printf("cannot reach node %d -- ID too big!\n", dest_id);
		pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));
		return -1;
	}else{
		pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));
	}
	if(dest_id == p_mem->local_id){
		printf("why would you send anything to yourself!?!\n");
		return -1;
	}
	pthread_mutex_lock(&(p_mem->mutexes->routing_mutex));
#ifdef DEBUG
	printf("trying to reach id %d\n", dest_id);
#endif
	int next_id = p_mem->p_routing_table->table[idToIndex(dest_id)].next_hop_id;
	if(next_id==-1){
#ifdef DEBUG
		printf("TODO: diff between totally unreachable and tmporarily unreachable node!\n");
#endif
		printf("cannot reach node %d. %s\n", dest_id, retry==RETRY?"Will try it later":"");
		pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));

		if(retry==RETRY){
			//pthread_mutex_lock(&(p_mem->mutexes->buffer_mutex));
			addWaitingMessage(dest_id, len, packet, p_mem);;
			//pthread_mutex_unlock(&(p_mem->mutexes->buffer_mutex));
		}
		return -1;
	}else{
		pthread_mutex_unlock(&(p_mem->mutexes->routing_mutex));
	}
	sendToNeighbour(next_id, packet, len, p_mem);
	return 0;
}
