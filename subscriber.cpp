#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "common.h"
#include "helpers.h"

void run_client(int sockfd) {
    sleep(1);
    send(sockfd, "Hello", 6, 0);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("\n Usage: %s <id> <ip> <port>\n", argv[0]);
    return 1;
  }

  /* parse subscriber id */
  char *id = argv[1];
  printf("%s\n", id);
  
  /* parse given port */
  uint16_t port;
  int rc = sscanf(argv[3], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  // Obtinem un socket TCP pentru conectarea la server
  const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "socket");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul
  // pentru conectare
  struct sockaddr_in serv_addr = { .sin_family = AF_INET,
                                   .sin_port = htons(port) };
  socklen_t socket_len = sizeof(struct sockaddr_in);

  rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
  DIE(rc < 0, "inet_pton");

  // Ne conectăm la server
  rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "connect");

  run_client(sockfd);

  rc = shutdown(sockfd, SHUT_RDWR);
  DIE(rc < 0, "shutdown");
  // Inchidem conexiunea si socketul creat
  close(sockfd);

  return 0;
}
