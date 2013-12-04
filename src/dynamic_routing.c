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

#define MAXBUFLEN 100

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int routingListen(void* port_number_ptr)
{
	int port_number = *( (int*)port_number_ptr);
	printf("port: %d\n", port_number);
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

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

	printf("listener: waiting to recvfrom...\n");

	addr_len = sizeof their_addr;
	for(;;){
		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
						(struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}

		printf("listener: got packet from %s\n",
				inet_ntop(their_addr.ss_family,
					get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
		printf("listener: packet is %d bytes long\n", numbytes);
		buf[numbytes] = '\0';
		printf("listener: packet contains \"%s\"\n", buf);
		if(sendto(sockfd,buf,MAXBUFLEN-1,0,(struct sockaddr *)&their_addr,sizeof(their_addr)) < 0){
			perror("odpoved");
		}

	}

	close(sockfd);

	return 0;
}

int sayHello(Connections conns)
{
	int i;
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

	int connection_count = conns.count;
	TConnection* connections = conns.array;


	for(i=0; i<connection_count; i++){
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;

		char port[7];
		sprintf(port, "%d", connections[i].port);

		if ((rv = getaddrinfo(connections[i].ip_address, port, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
		}

		// loop through all the results and make a socket
		for(p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype,
							p->ai_protocol)) == -1) {
				perror("talker: socket");
				continue;
			}

			break;
		}

		if (p == NULL) {
			fprintf(stderr, "talker: failed to bind socket\n");
			return 2;
		}

		if ((numbytes = sendto(sockfd, "Hello there!", strlen("Hello there!"), 0,
						p->ai_addr, p->ai_addrlen)) == -1) {
			perror("talker: sendto");
			exit(1);
		}

		freeaddrinfo(servinfo);

		printf("talker: sent %d bytes to %d\n", numbytes, connections[i].id);
		close(sockfd);
	}
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
		printf("trying to connect to %s:%s\n", out_conns.array[i].ip_address, port_str);
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

		freeaddrinfo(result);           /* No longer needed */

		/* Send remaining command-line arguments as separate
		   datagrams, and read responses from server */
		//printf("created socket: %d\n", sfd);

		struct mem_and_sfd *param;
		param = (struct mem_and_sfd*) malloc(sizeof(struct mem_and_sfd));
		param->sfd = sfd;
		param->mem = mem;

		pthread_t listen_thread;
		pthread_create(&listen_thread, NULL, (void*) &sockListener, (void*) param);

		sendto(sfd, "Ahojky!", strlen("Ahojky!")+1, 0, 0, 0);

		printf("debug: %d\n", out_conns.array[i].id);
		printf("dalsi: %d\n", mem->p_routing_table->table[4].next_hop_id);
		struct real_connection *rc = &(mem->p_connections[out_conns.array[i].id]);
		rc->type = OUT_CONN; // OUT connection
		rc->id = out_conns.array[i].id;
		rc->last_seen = clock();
		rc->online = 0;



	}
}

void sockListener(void *param)
{
	printf("starting new listener...\n");
	int sfd = ((struct mem_and_sfd*)param)->sfd;
	//printf("using listening socket: %d\n", sfd);
	int s;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len = sizeof(struct sockaddr_storage);
	ssize_t nread;
	char buf[BUF_SIZE];

	for (;;) {
		nread = recvfrom(sfd, buf, BUF_SIZE, 0,
				(struct sockaddr *) &peer_addr, &peer_addr_len);
		if (nread == -1)
			continue;               /* Ignore failed request */

		char host[NI_MAXHOST], service[NI_MAXSERV];

		s = getnameinfo((struct sockaddr *) &peer_addr,
				peer_addr_len, host, NI_MAXHOST,
				service, NI_MAXSERV, NI_NUMERICSERV);
		if (s == 0){
			printf("Received %ld bytes from %s:%s\n",
					(long) nread, host, service);
			printf("got: %s\n", buf);
		}else{
			fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
		}

		/*
		 * if (sendto(sfd, buf, nread, 0,
		 (struct sockaddr *) &peer_addr,
		 peer_addr_len) != nread)
		 fprintf(stderr, "Error sending response\n");
		 */
		//sleep(5);
	}
}

