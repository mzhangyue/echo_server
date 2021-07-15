/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 
#define BUFFER_SIZE 1024 // max number of bytes can send and receive

void chatFunc(int sockfd) { 
	char buf[BUFFER_SIZE]; // send and receive buffer
	int n, rv;
	while(1) {
		// clear buffer
		bzero(buf, sizeof(buf));
		// fill buffer with user input from command line
		printf("Enter input: ");
		n = 0;
		while ((buf[n++] = getchar()) != '\n');
		// send data
		if ((rv = send(sockfd, buf, n, 0)) == -1) {
			perror("send");
		}
		// clear buffer for recv
		bzero(buf, sizeof(buf));
		// receive response
		if ((rv = recv(sockfd, buf, BUFFER_SIZE, 0)) == -1) {
			perror("read");
		}
		// print response
		printf("From Server: %s", buf);
		// typing exit will end connection
		if ((strncmp(buf, "exit", 4)) == 0) {
			printf("Client Exit\n");
			break;
		}
	}

}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd; // storing socket file descriptor
	struct addrinfo hints, *servinfo, *p; // used for prepping sockaddr structs
	int rv; // return value for error checking
	char s[INET6_ADDRSTRLEN]; // used to store IP addresses
	// input checking
	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints); // clear hints struct
	hints.ai_family = AF_UNSPEC; // dont care IPv4 or v6
	hints.ai_socktype = SOCK_STREAM; // TCP

	// fills out servinfo as linked list of posible socket address structures
	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// get socket file descriptor
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		// connect to socket, ready to send and recv
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}
	// if none of the results resulted in connect
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	// print out IP address of server
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	// start chatting
	chatFunc(sockfd);

	// done with connection
	close(sockfd);

	return 0;
}
