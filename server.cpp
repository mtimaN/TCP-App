#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <vector>

#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 32

int receive_and_send(int connfd1, int connfd2, size_t len) {
	int bytes_received;
	char buffer[len];

	bytes_received = recv_all(connfd1, buffer, len);
	if (bytes_received == 0) {
		return 0;
	}
	DIE(bytes_received < 0, "recv");

	// Trimitem mesajul catre connfd2
	int rc = send_all(connfd2, buffer, len);
	if (rc <= 0) {
		perror("send_all");
		return -1;
	}

	return bytes_received;
}

void run_server(const int tcp_fd, const int udp_fd) {
	std::vector<struct pollfd> poll_fds(3);
	int rc;

	rc = listen(tcp_fd, MAX_CONNECTIONS);
	DIE(rc < 0, "listen");

	/* Adding into the pollfd array entries for listening to tcp,  
	 * udp and stdin(in case it receives an exit command)
	 */
	poll_fds[0] = (struct pollfd) {tcp_fd, POLLIN, 0};
	poll_fds[1] = (struct pollfd) {udp_fd, POLLIN, 0};
	poll_fds[2] = (struct pollfd) {STDIN_FILENO, POLLIN, 0};

	while (1) {
		rc = poll(poll_fds.data(), poll_fds.size(), -1);
		DIE(rc < 0, "poll");

		for (int i = 0; i < poll_fds.size(); i++) {
			if (poll_fds[i].revents & POLLIN) {
				if (poll_fds[i].fd == tcp_fd) {
					/* connection received */
					struct sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					const int newsockfd =
							accept(tcp_fd, (struct sockaddr *)&cli_addr, &cli_len);
					DIE(newsockfd < 0, "accept");

					/* add new sockfd to poll */
					poll_fds.push_back({newsockfd, POLLIN, 0});

					/* disabling Nagel's algorithm */
					if (setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
						perror("setsockopt(TCP_NODELAY) failed");
					}

					printf("New client %s connected from: %s:%hd.\n", "bla bla",
					 inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
				} else if (poll_fds[i].fd == udp_fd) {
					// new UDP client
					// struct sockaddr_in cli_addr;
					// socklen_t cli_len = sizeof(cli_addr);
					// const int newsockfd = accept(udp_fd,)
				} else if (poll_fds[i].fd == STDIN_FILENO) {
					// stdin command received
					char buff[64];
					fgets(buff, 64, stdin);
					if (strcmp(buff, "exit") == 0) {
						exit(0);
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("\n Usage: %s <port>\n", argv[0]);
		return 1;
	}

	uint16_t port;
	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	/* get TCP socket for listening to clients */
	const int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_fd < 0, "socket");

	/* get UDP socket for listening to clients */
	const int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp_fd < 0, "socket");

	struct sockaddr_in serv_addr = { .sin_family = AF_INET,
									 .sin_port   = htons(port),
									 .sin_addr   = { .s_addr = INADDR_ANY }};
	socklen_t len = sizeof(struct sockaddr_in);

	int enable = 1;

	/* see: https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux */
	if (setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("setsockopt(SO_REUSEADDR) failed");
	}

	if (setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("setsockopt(SO_REUSEADDR) failed");
	}

	rc = bind(tcp_fd, (const struct sockaddr *)&serv_addr, len);
	DIE(rc < 0, "bind");

	rc = bind(udp_fd, (const struct sockaddr *)&serv_addr, len);
	DIE(rc < 0, "bind");

	run_server(tcp_fd, udp_fd);

	/* close sockets */
	close(tcp_fd);
	close(udp_fd);

	return 0;
}
