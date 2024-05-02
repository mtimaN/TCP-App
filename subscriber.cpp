#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <cstdio>

#include "common.h"
#include "helpers.h"
#include "parser.h"

void run_client(int sockfd) {
	struct pollfd poll_fds[] = { {STDIN_FILENO, POLLIN, 0}, {sockfd, POLLIN, 0} };
	int rc;

	const int size = sizeof(poll_fds) / sizeof(*poll_fds);
	char buf[MAX_UDP_PAYLOAD] = {0};

    while (1) {
		poll(poll_fds, size, -1);

		if (poll_fds[0].revents & POLLIN) {
			fgets(buf, sizeof(buf), stdin);
			buf[strlen(buf) - 1] = '\0';
			if (!strncmp(buf, "exit", 4)) {
				int32_t buf_len = 0;
				int rc = send_all(poll_fds[1].fd, &buf_len, sizeof(buf_len));
				DIE(rc < 0, "send");
				return;
			} else if (!strncmp(buf, "subscribe", sizeof("subscribe") - 1)) {
				char *off_buf = buf + sizeof("subscribe") - 1;
				int32_t buf_len = strlen(off_buf);
				off_buf[0] = 's';
	
				rc = send_all(poll_fds[1].fd, &buf_len, sizeof(buf_len));
				DIE(rc < 0, "send");

				rc = send_all(poll_fds[1].fd, off_buf, buf_len);
				DIE(rc < 0, "send");
				printf("Subscribed to topic %s\n", off_buf + 1);
			} else if (!strncmp(buf, "unsubscribe", sizeof("unsubscribe") - 1)) {
				char *off_buf = buf + sizeof("unsubscribe") - 1;
				int32_t buf_len = strlen(off_buf);
				off_buf[0] = 'u';

				rc = send_all(poll_fds[1].fd, &buf_len, sizeof(buf_len));
				DIE(rc < 0, "send");

				rc = send_all(poll_fds[1].fd, off_buf, buf_len);
				DIE(rc < 0, "send");
				printf("Unsubscribed from topic %s\n", off_buf + 1);
			}
		} else if (poll_fds[1].revents & POLLIN) {
			int32_t buf_len;
			rc = recv_all(poll_fds[1].fd, &buf_len, sizeof(buf_len));
			DIE(rc < 0, "recv");
			if (buf_len == 0) {
				printf("Server closed\n");
				return;
			}
			rc = recv_all(poll_fds[1].fd, buf, buf_len);
			parse_notification(buf);
		}
    }
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		printf("\n Usage: %s <id> <ip> <port>\n", argv[0]);
		return 1;
	}

	/* disable I/O buffering */
	setvbuf(stdout, NULL, _IONBF, 0);

	/* parse subscriber id */
	char *id = argv[1];
	
	/* parse given port */
	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	/* obtain a TCP socket for connecting to the server */
	const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	/* fill in serv_addr with server address */
	struct sockaddr_in serv_addr = { .sin_family = AF_INET,
									 .sin_port = htons(port),
									 .sin_addr = { .s_addr = 0 },
									 .sin_zero = {0} };
	socklen_t socket_len = sizeof(struct sockaddr_in);

	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc < 0, "inet_pton");

	int enable = 1;
	/* disabling Nagel's Algorithm */
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
		perror("setsockopt(TCP_NODELAY) failed");
	}

	/* make socket reusable */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("setsockopt(SO_REUSEADDR) failed");
	}

	rc = connect(sockfd, (struct sockaddr *)&serv_addr, socket_len);
	DIE(rc < 0, "connect");

	/* send ID to the server */
	int32_t id_size = strlen(id);
	rc = send_all(sockfd, &id_size, sizeof(id_size));
	DIE(rc < 0, "send");
	rc = send_all(sockfd, id, id_size);
	DIE(rc < 0, "send");

	run_client(sockfd);

	/* close the connection */
	close(sockfd);

	return 0;
}
