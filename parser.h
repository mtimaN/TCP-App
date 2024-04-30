/**
 * @file parser.h
 * @author Matei Mantu (matei.mantu1@gmail.com)
 * @brief parsing functions for UDP and TCP messages
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#include "common.h"

/**
 * @brief possible data types for UDP transmissions
 */
enum DATA_TYPE : char { INT, SHORT_REAL, FLOAT, STRING };


void process_tcp_login(std::vector<struct pollfd> &poll_fds, std::vector<subscriber_t> &subscribers, int tcp_fd);

/**
 * @brief parser for udp client requests
 * Parser for messages from UDP clients. The messages are formatted as follows:
 *  - topic: 50 bytes at max.
 *  - data type: one byte
 *  - content: 1500 bytes at max
 * 
 * @param udp_fd socket fd for the udp connection
 */
void process_udp_message(std::vector<subscriber_t> &subscribers, const int udp_fd);

void parse_notification(const char *buffer);
