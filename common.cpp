/**
 * @file common.cpp
 * @author Matei Mantu (matei.mantu1@gmail.com)
 * @brief helper functions implemented during PCOM lab, useful for both server
 * and client
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

int recv_all(int sockfd, void *buffer, size_t len) {
	char *buff = (char *)buffer;

	while (len) {
		ssize_t ret = recv(sockfd, buff, len, 0);

		if (ret == -1) {
		  return -1;
		}

		/* connection closed abruptly. ending recv_all */
		if (ret == 0) {
			*(int32_t *)buffer = 0;
			return 0;
		}

		buff += ret;
		len -= ret;
	}

	return 0;
}

int send_all(int sockfd, void *buffer, size_t len) {
	char *buff = (char *)buffer;

	while (len) {
		ssize_t ret = send(sockfd, buff, len, 0);

		if (ret == -1) {
		  return -1;
		}

		buff += ret;
		len -= ret;
  	}

	return 0;
}
