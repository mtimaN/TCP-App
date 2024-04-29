/**
 * @file parser.cpp
 * @author Matei Mantu (matei.mantu1@gmail.com)
 * @brief parsing functions for UDP and TCP messages
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <cstdio>
#include <cmath>
#include <vector>
#include <cstring>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>
#include <netinet/tcp.h>
#include <unistd.h>


#include "common.h"
#include "parser.h"
#include "helpers.h"

void process_tcp_login(std::vector<struct pollfd> &poll_fds, std::vector<subscriber_t> &subscribers, int tcp_fd) {
	/* connection received */
	struct sockaddr_in cli_addr;
	socklen_t cli_len = sizeof(cli_addr);
	const int newsockfd =
			accept(tcp_fd, (struct sockaddr *)&cli_addr, &cli_len);
	DIE(newsockfd < 0, "accept");

	/* add new sockfd to poll */
	poll_fds.push_back({newsockfd, POLLIN, 0});

	int enable = 1;
	/* disabling Nagel's algorithm */
	if (setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
		perror("setsockopt(TCP_NODELAY) failed");
	}

	char id_buffer[MAX_ID_SIZE];
	bool name_in_use = false;
	int rc = recv(newsockfd, id_buffer, sizeof(id_buffer), 0);
	DIE(rc <= 0, "recv");

	for (auto &it: subscribers) {
		printf("%s\n", it.id);
		if (strcmp(it.id, id_buffer) == 0) {
			if (it.online == true) {
				printf("Client %s already connected\n", it.id);
				rc = close(newsockfd);
				poll_fds.pop_back();
			} else {
				it.online = true;
				it.socketfd = newsockfd;
			}
			name_in_use = true;
			break;
		}
	}

	if (name_in_use) {
		return;
	}

	subscriber_t sub;
	memset((void*)&sub, 0, sizeof(sub));
	strcpy(sub.id, id_buffer);
	sub.socketfd = newsockfd;
	sub.online = true;

	subscribers.push_back(sub);
	printf("New client %s connected from: %s:%hu.\n", sub.id,
		inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
}

void parse_udp_message(const int udp_fd) {

	// UDP message
	char buffer[MAX_UDP_PAYLOAD];
	int rc = recv(udp_fd, buffer, MAX_UDP_PAYLOAD, 0);
	DIE(rc < 0, "recv");

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
}