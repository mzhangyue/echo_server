/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXCONNECT 5 // max number of concurrent connections allowed
#define BUFFER_SIZE 1024 // size of buffer

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// echo function for each thread
void *echo(void *newfd_ptr) {
	int new_fd = *(int*) newfd_ptr;
	// create buffer to receive incoming msgs
	char buf[BUFFER_SIZE];
	// echo back incoming msgs
	while (1) {
		int read = recv(new_fd, buf, BUFFER_SIZE, 0);
		if (read == 0) {
			printf("Remote side closed\n");
			close(new_fd);
			break;
		} else if (read == -1) {
			perror("recv");
			continue;
		}
		// echo back received msg
		if (send(new_fd, buf, read, 0) == -1) {
			perror("send");
		}
	}
	pthread_exit(NULL);
	return NULL;
}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p; // used for prepping sockaddr structs
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	pthread_t threads[MAXCONNECT]; // array of thread ids
	int rc; // return code
	int t = 0; // next index of threads to assign
	int yes=1;
	char s[INET6_ADDRSTRLEN]; // used to contain IP address
	int rv; // return value for error checking


	memset(&hints, 0, sizeof hints); // clear hints struct
	hints.ai_family = AF_UNSPEC; // dont care IPv4 or v6
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE; // use my IP

	// fills out servinfo as linked list of posible socket address structures
	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) { 
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// get socket file descriptor
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		// allows reuse of local addresses
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		// bind port number to socket
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure
	// if none of the results resulted in bind
	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
	// wait for incoming connections with queue size of BACKLOG
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		// put incoming connection info into their_addr
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}
		// print out incoming address
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		// create new thread for the connection
		rc = pthread_create(&threads[t], NULL, echo, &new_fd);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
		t++;
	}
	pthread_exit(NULL); // wait for threads
	return 0;
}
