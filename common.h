#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>

/**
 * @brief send exactly len bytes from buffer
 * 
 * @param sockfd socket file descriptor
 * @param buff given buffer
 * @param len bytes
 * @return int 0 if succeeded, -1 otherwise
 */
int send_all(int sockfd, void *buff, size_t len);

/**
 * @brief receive exactly len bytes and store them into the buffer
 * 
 * @param sockfd socket file descriptor
 * @param buff given buffer
 * @param len bytes
 * @return int 0 if succeeded, -1 otherwise
 */
int recv_all(int sockfd, void *buff, size_t len);

/* max message size */
#define MSG_MAXSIZE 1024

typedef struct subscribe {
  char topic[51];
} subscribe_t;

#endif
