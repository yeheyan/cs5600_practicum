/*
 * network.h, Yehen Yan, CS5600 Practicum II
 * Network communication function declarations for remote file system
 * Last modified: Dec 2025
 */

#ifndef NETWORK_H
#define NETWORK_H

/**
 * @brief Create and connect socket to server
 *
 * @param server_ip Server IP address
 * @param port Port number
 * @return int Socket file descriptor on success, -1 on failure
 */
int connect_to_server(const char *server_ip, int port);

/**
 * @brief Send operation type
 *
 * @param sock Socket file descriptor
 * @param operation
 * @return int 0 on success, -1 on failure
 */
int send_operation(int sock, const char *operation);

/**
 * @brief Send a string
 *
 * @param sock Socket file descriptor
 * @param str String to send
 * @return int 0 on success, -1 on failure
 */
int send_string(int sock, const char *str);

/**
 * @brief Send a file
 *
 * @param sock Socket file descriptor
 * @param filename Name of the file to send
 * @return long Number of bytes sent, -1 on failure
 */
long send_file(int sock, const char *filename);

/**
 * @brief Receive a file
 *
 * @param sock Socket file descriptor
 * @param filename Name of the file to receive
 * @return long Number of bytes received, -1 on failure
 */
long receive_file(int sock, const char *filename);

/**
 * @brief Receive a response
 *
 * @param sock Socket file descriptor
 * @param buffer Buffer to store the received response
 * @param buffer_size Size of the buffer
 * @return int Number of bytes received, -1 on failure
 */
int receive_response(int sock, char *buffer, size_t buffer_size);

#endif