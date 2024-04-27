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
// #include <vector>
#include <math.h>

#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 32
#define MAX_UDP_PAYLOAD 1551
#define MAX_COMMAND_LEN 5

enum DATA_TYPE : char { INT, SHORT_REAL, FLOAT, STRING };

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
					poll_fds.push_back({newsockfd, POLLIN | POLLHUP, 0});

					int32_t enable = 1;
					/* disabling Nagel's algorithm */
					if (setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
						perror("setsockopt(TCP_NODELAY) failed");
					}

					printf("New client %s connected from: %s:%hu.\n", "bla bla",
					 inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
				} else if (poll_fds[i].fd == udp_fd) {
					// UDP message
					char buffer[MAX_UDP_PAYLOAD];
					int rc = recv(udp_fd, buffer, MAX_UDP_PAYLOAD, 0);

					const char *topic = buffer;
					const uint32_t topic_len = 50;
					const uint8_t *data_type = (uint8_t *)topic + topic_len;
					printf("data type: %hhd ", *data_type);
					const int8_t *content = (int8_t *)data_type + 1;

					switch (*data_type) {
						case INT: {
							const uint8_t *sign = (uint8_t *)content;
							const uint32_t *value = (uint32_t *)(sign + 1);
							char result[20];
							if (*sign == 1) {
							    snprintf(result, sizeof(result), "-%u", ntohl(*value));
							} else {
							    snprintf(result, sizeof(result), "%u", ntohl(*value));
							}
							printf("%s\n", result);
							break;
						}
							break;
						case SHORT_REAL: {
							char result[20];
							const uint16_t *value = (uint16_t *)content;
							snprintf(result, sizeof(result), "%.2f", (float)ntohs(*value) / 100);
							printf("%s\n", result);
							break;
						}
							break;
						case FLOAT: {
							const uint8_t *sign = (uint8_t *)content;
							const uint32_t *value = (uint32_t *)(sign + 1);
							const uint8_t *power = (uint8_t *)(value + 1);
							float parsed_float = ntohl(*value) * pow(10, -(*power));

							if (*sign == 1) {
								parsed_float *= -1;
							}
							printf("%.*f\n", *power, parsed_float);
							break;
						}
						case STRING: {
							printf("%.1500s\n", (char *)content);
							break;
						}
						default: {
							printf("Unrecognized format\n");
							break;
						}
					}
				} else if (poll_fds[i].fd == STDIN_FILENO) {
					// stdin command received
					char buff[MAX_COMMAND_LEN];
					fgets(buff, MAX_COMMAND_LEN, stdin);
					if (strcmp(buff, "exit") == 0) {
						exit(0);
					}
				} else {
					/* woah? a client? */
					char buffer[BUFSIZ];
					int rc = recv(poll_fds[i].fd, buffer, 1000, 0);
					DIE(rc < 0, "recv");
					if (rc == 0) {
						printf("connection closed\n");
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
									 .sin_addr   = { .s_addr = INADDR_ANY }};
	socklen_t len = sizeof(struct sockaddr_in);

	int enable = 1;

	/* see: https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux */
	if (setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("setsockopt(SO_REUSEADDR) failed");
	}

	/* disabling Nagel's Algorithm */
	if (setsockopt(tcp_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
    	perror("setsockopt(TCP_NODELAY) failed");
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
