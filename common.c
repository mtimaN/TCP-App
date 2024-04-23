#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

int recv_all(int sockfd, void *buffer, size_t len) {
	char *buff = buffer;

	while (len) {
		size_t ret = recv(sockfd, buff, len, 0);

		if (ret == -1) {
		  return -1;
		}

		buff += ret;
		len -= ret;
	}

	return 0;
}

int send_all(int sockfd, void *buffer, size_t len) {
	char *buff = buffer;

	while (len) {
		size_t ret = send(sockfd, buff, len, 0);

		if (ret == -1) {
		  return -1;
		}

		buff += ret;
		len -= ret;
  	}

	return 0;
}
