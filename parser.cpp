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
#define MAX_CONTENT_LEN 1600

bool topic_match(char *regex, char *topic) {
	char *tokens_regex[MAX_TOPICS], *tokens_topic[MAX_TOPICS];

	bool res = false;
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
				res = true;
				goto cleanup;
			}

			while (tokens_topic[j] != NULL && strcmp(tokens_regex[i], tokens_topic[j])) {
				j++;
			}

			if (tokens_topic[j] == NULL) {
				res = true;
				goto cleanup;
			}

		} else if (strcmp(tokens_regex[i], "+")) {
			if (strcmp(tokens_regex[i], tokens_topic[j])) {
				goto cleanup;
			}
		}
		i++;
		j++;
	}

	if (tokens_regex[i] == NULL && tokens_topic[j] == NULL) {
		res = true;
	}

cleanup:
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
	/* making socket reusable */
	if (setsockopt(newsockfd, SOL_TCP, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("setsockopt(REUSEADDR) failed");
	}

	/* disabling Nagel's algorithm */
	if (setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
		perror("setsockopt(TCP_NODELAY) failed");
	}

	char id_buffer[MAX_ID_SIZE];
	int32_t id_len;
	int rc = recv_all(newsockfd, &id_len, sizeof(id_len));
	DIE(rc < 0, "recv");
	rc = recv_all(newsockfd, id_buffer, id_len);
	DIE(rc < 0, "recv");

	for (auto &it: subscribers) {
		if (strcmp(it.id, id_buffer) == 0) {
			if (it.online == true) {
				printf("Client %s already connected.\n", it.id);
				rc = close(newsockfd);
				poll_fds.pop_back();
			} else {
				it.online = true;
				it.socketfd = newsockfd;
				printf("New client %s connected from: %s:%hu.\n", it.id,
					inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
			}
			return;
		}
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
	char buffer[MAX_UDP_PAYLOAD + UDP_OFFSET] = {0};
	struct sockaddr_in src_addr = {0, 0, 0, 0};
	socklen_t addrlen = sizeof(src_addr);
	int32_t buffer_len = recvfrom(udp_fd, buffer + UDP_OFFSET, MAX_UDP_PAYLOAD, 0, (struct sockaddr *)&src_addr, &addrlen);
	DIE(buffer_len < 0, "recv");
	buffer_len += UDP_OFFSET;

	memcpy(buffer, &src_addr.sin_addr, sizeof(src_addr.sin_addr));
	memcpy(buffer + sizeof(src_addr.sin_addr), &src_addr.sin_port, sizeof(src_addr.sin_port));

	char formatted_topic[TOPIC_LEN + 1] = {0};
	strncpy(formatted_topic, buffer + UDP_OFFSET, TOPIC_LEN);
	for (const auto &sub: subscribers) {
		for (const auto &regex: sub.topics) {
			char *regex_cstr = strdup(regex.c_str());
			char *topic_copy = strdup(formatted_topic);
			if (topic_match(regex_cstr, topic_copy)) {
				rc = send_all(sub.socketfd, &buffer_len, sizeof(int));
				DIE(rc < 0, "send");
				rc = send_all(sub.socketfd, buffer, buffer_len);
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
	int offset = 0;
	const char *topic = buffer + UDP_OFFSET;
	const uint8_t *data_type = (uint8_t *)topic + TOPIC_LEN;
	const int8_t *content = (int8_t *)data_type + 1;
	char result[MAX_CONTENT_LEN + 1] = {0};
	snprintf(result, sizeof(result), "%s", inet_ntoa(*(struct in_addr *)buffer));
	offset = strlen(result);
	result[offset++] = ':';
	snprintf(result + offset, sizeof(result), "%hu", *(uint16_t *)(buffer + sizeof(uint32_t)));
	offset = strlen(result);
	snprintf(result + offset, TOPIC_LEN + 7, " - %s - ", topic);
	offset += std::min(strlen(topic), (size_t)TOPIC_LEN) + 6;
	switch (*data_type) {
		case INT: {
			const uint8_t *sign = (uint8_t *)content;
			const uint32_t *value = (uint32_t *)(sign + 1);
			if (*sign == 1) {
				snprintf(result + offset, sizeof(result), "INT - -%u", ntohl(*value));
			} else {
				snprintf(result + offset, sizeof(result), "INT - %u", ntohl(*value));
			}
			break;
		}
		case SHORT_REAL: {
			const uint16_t *value = (uint16_t *)content;
			snprintf(result + offset, sizeof(result), "SHORT_REAL - %.2f", (float)ntohs(*value) / 100);
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
			snprintf(result + offset, sizeof(result), "FLOAT - %.*f", *power, parsed_float);
			break;
		}
		case STRING: {
			snprintf(result + offset, sizeof(result), "STRING - %.1499s", (char *)content);
			break;
		}
		default: {
			printf("Unrecognized format\n");
			return;
		}
	}

	printf("%s\n", result);
}