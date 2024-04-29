#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <cstdio>

#include "common.h"
#include "helpers.h"

void run_client(int sockfd) {
	send(sockfd, "Hello", 6, 0);
    while (1) {
    }
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
	printf("\n Usage: %s <id> <ip> <port>\n", argv[0]);
	return 1;
	}

	/* parse subscriber id */
	char *id = argv[1];
	printf("%s\n", id);
	
	/* parse given port */
	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	/* obtain a TCP socket for connecting to the server */
	const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	/* fill in serv_addr with server address*/
	struct sockaddr_in serv_addr = { .sin_family = AF_INET,
									 .sin_port = htons(port),
									 .sin_addr = { .s_addr = 0 },
									 .sin_zero = {0} };
	socklen_t socket_len = sizeof(struct sockaddr_in);

	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc < 0, "inet_pton");

	int enable = 1;
	/* disabling Nagel's Algorithm */
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
		perror("setsockopt(TCP_NODELAY) failed");
	}

	rc = connect(sockfd, (struct sockaddr *)&serv_addr, socket_len);
	DIE(rc < 0, "connect");

	/* send ID to the server */
	rc = send(sockfd, id, sizeof(id), 0);

	run_client(sockfd);

	/* close the connection */
	close(sockfd);

	return 0;
}
