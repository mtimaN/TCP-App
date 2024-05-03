/**
 * @file common.h
 * @author Matei Mantu (matei.mantu1@gmail.com)
 * @brief helper functions implemented during PCOM lab, useful for both server
 * and client
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <string>

#define MAX_ID_SIZE 11
#define MAX_CONNECTIONS 32
#define MAX_UDP_PAYLOAD 1557
#define UDP_OFFSET 6
#define MAX_COMMAND_LEN 20

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

/**
 * @brief information regarding a subscriber
 * 
 */
typedef struct subscriber {
	int socketfd;
	char id[MAX_ID_SIZE];
	bool online;
	std::vector<std::string> topics;
} subscriber_t;
