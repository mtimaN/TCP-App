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
#include "parser.h"


void run_server(const int tcp_fd, const int udp_fd) {
	std::vector<struct pollfd> poll_fds(3);
	std::vector<subscriber_t> subscribers;
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

		for (uint32_t i = 0; i < poll_fds.size(); ++i) {
			if (poll_fds[i].revents & POLLIN) {
				if (poll_fds[i].fd == tcp_fd) {
					process_tcp_login(poll_fds, subscribers, tcp_fd);
				} else if (poll_fds[i].fd == udp_fd) {
					parse_udp_message(udp_fd);
				} else if (poll_fds[i].fd == STDIN_FILENO) {
					/* stdin command received */
					char buff[MAX_COMMAND_LEN];
					fgets(buff, MAX_COMMAND_LEN, stdin);
					if (strcmp(buff, "exit") == 0) {
						return;
					}
				} else {
					/* woah? a client? */
					char buffer[BUFSIZ];
					rc = recv(poll_fds[i].fd, buffer, sizeof(buffer), 0);
					DIE(rc < 0, "recv");
					if (rc == 0) {
						printf("connection closed\n");
						for (auto &sub: subscribers) {
							if (sub.socketfd == poll_fds[i].fd) {
								sub.online = false;
								break;
							}
						}
						rc = close(poll_fds[i].fd);
						DIE(rc < 0, "close");
						poll_fds.erase(poll_fds.begin() + i);
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

	/* disable buffering I/O */
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

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
									 .sin_addr   = { .s_addr = INADDR_ANY },
									 .sin_zero   = {0} };
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