/**
 * @file parser.c
 * @author Matei Mantu (matei.mantu1@gmail.com)
 * @brief 
 * Parser for messages from UDP clients. The messages are formatted as follows:
 *  - topic: 50 bytes at max.
 *  - data type: one byte
 *  - content: 1500 bytes at max
 * 
 * @copyright Copyright (c) 2024
 * 
 */

