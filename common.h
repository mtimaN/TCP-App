#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>

#define MAX_ID_SIZE 10

/**
 * @brief send exactly len bytes from buffer
 * 
 * @param sockfd socket file descriptor
 * @param buff given buffer
 * @param len bytes
 * @return int 0 if succeeded, -1 otherwise
 */
int send_all(int sockfd, void *buffer, size_t len);

/**
 * @brief receive exactly len bytes and store them into the buffer
 * 
 * @param sockfd socket file descriptor
 * @param buff given buffer
 * @param len bytes
 * @return int 0 if succeeded, -1 otherwise
 */
int recv_all(int sockfd, void *buffer, size_t len);

typedef struct subscriber {
	int socketfd;
	char id[MAX_ID_SIZE];
	bool online;
	std::vector<char*> topics;
} subscriber_t;