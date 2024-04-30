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

#define MAX_TOPICS 25
#define TOPIC_LEN 50
#define MAX_CONTENT_LEN 1500

bool topic_match(char *regex, char *topic) {
	char *tokens_regex[MAX_TOPICS], *tokens_topic[MAX_TOPICS];

	char *token = strtok(regex, "/");
	int i = 0;
	while (token != NULL) {
		tokens_regex[i++] = strdup(token);
		token = strtok(NULL, "/");
	}
	tokens_regex[i] = NULL;

	token = strtok(topic, "/");
	i = 0;
	while (token != NULL) {
		tokens_topic[i++] = strdup(token);
		token = strtok(NULL, "/");
	}
	tokens_topic[i] = NULL;

	i = 0;
	int j = 0;
	while (tokens_regex[i] && tokens_topic[j]) {
		if (!strcmp(tokens_regex[i], "*")) {
			i++;
			if (tokens_regex[i] == NULL) {
				return true;
			}

			while (tokens_topic[j] != NULL && strcmp(tokens_regex[i], tokens_topic[j])) {
				j++;
			}

			if (tokens_topic[j] == NULL) {
				return false;
			}
		} else if (strcmp(tokens_regex[i], "+")) {
			if (strcmp(tokens_topic[i], tokens_topic[j])) {
				return false;
			}
		}
		i++;
		j++;
	}

	bool res = false;
	if (tokens_regex[i] == NULL && tokens_topic[j] == NULL) {
		res = true;
	}

	for (i = 0; tokens_regex[i]; ++i) {
		free(tokens_regex[i]);
	}

	for (j = 0; tokens_topic[j]; ++j) {
		free(tokens_topic[j]);
	}

	return res;
}

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
				printf("Client %s already connected.\n", it.id);
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

void process_udp_message(std::vector<subscriber_t> &subscribers, const int udp_fd) {

	// UDP message
	int rc;
	char buffer[MAX_UDP_PAYLOAD] = {0};
	struct sockaddr_in src_addr = {0, 0, 0, 0};
	socklen_t addrlen = sizeof(src_addr);
	int buffer_len = recvfrom(udp_fd, buffer + UDP_OFFSET, MAX_UDP_PAYLOAD, 0, (struct sockaddr *)&src_addr, &addrlen);
	DIE(buffer_len < 0, "recv");

	memcpy(buffer, &src_addr.sin_addr, sizeof(src_addr.sin_addr));
	memcpy(buffer + sizeof(src_addr.sin_addr), &src_addr.sin_port, sizeof(src_addr.sin_port));

	char formatted_topic[TOPIC_LEN + 1] = {0};
	strncpy(formatted_topic, buffer + UDP_OFFSET, TOPIC_LEN);
	for (const auto &sub: subscribers) {
		for (const auto &regex: sub.topics) {
			char *regex_cstr = strdup(regex.c_str());
			char *topic_copy = strdup(formatted_topic);
			if (topic_match(regex_cstr, topic_copy)) {
				rc = send(sub.socketfd, buffer, buffer_len, 0);
				DIE(rc < 0, "send");
				free(regex_cstr);
				free(topic_copy);
				break;
			}
			free(regex_cstr);
			free(topic_copy);
		}
	}
}

void parse_notification(const char *buffer) {
	const char *topic = buffer + UDP_OFFSET;
	const uint8_t *data_type = (uint8_t *)topic + TOPIC_LEN;
	const int8_t *content = (int8_t *)data_type + 1;
	char result[MAX_CONTENT_LEN + 1] = {0};

	switch (*data_type) {
		case INT: {
			const uint8_t *sign = (uint8_t *)content;
			const uint32_t *value = (uint32_t *)(sign + 1);
			if (*sign == 1) {
				snprintf(result, sizeof(result), "-%u", ntohl(*value));
			} else {
				snprintf(result, sizeof(result), "%u", ntohl(*value));
			}
			break;
		}
		case SHORT_REAL: {
			const uint16_t *value = (uint16_t *)content;
			snprintf(result, sizeof(result), "%.2f", (float)ntohs(*value) / 100);
			break;
		}
		case FLOAT: {
			const uint8_t *sign = (uint8_t *)content;
			const uint32_t *value = (uint32_t *)(sign + 1);
			const uint8_t *power = (uint8_t *)(value + 1);
			float parsed_float = ntohl(*value) * pow(10, -(*power));

			if (*sign == 1) {
				parsed_float *= -1;
			}
			snprintf(result, sizeof(result), "%.*f\n", *power, parsed_float);
			break;
		}
		case STRING: {
			snprintf(result, sizeof(result), "%.1499s\n", (char *)content);
			break;
		}
		default: {
			printf("Unrecognized format\n");
			return;
		}
	}

	printf("%s\n", result);
}