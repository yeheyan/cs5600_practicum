/*
 * network.h, Yehen Yan, CS5600 Practicum II
 * Network communication function declarations for remote file system
 * Last modified: Dec 2025
 */
#ifndef NETWORK_H
#define NETWORK_H

#include <stdio.h>

/**
 * @brief Reliably send all data (handles partial sends)
 * @param sock Socket file descriptor
 * @param data Data to send
 * @param len Length of data
 * @return int 0 on success, -1 on failure
 */
int send_all(int sock, const void *data, size_t len);

/**
 * @brief Reliably receive all data (handles partial receives)
 * @param sock Socket file descriptor
 * @param buffer Buffer to store received data
 * @param len Expected length of data
 * @return int 0 on success, -1 on failure
 */
int recv_all(int sock, void *buffer, size_t len);

/**
 * @brief Create and connect socket to server
 * @param server_ip Server IP address
 * @param port Port number
 * @return int Socket file descriptor on success, -1 on failure
 */
int connect_to_server(const char *server_ip, int port);

/**
 * @brief Send operation type
 * @param sock Socket file descriptor
 * @param operation Operation string
 * @return int 0 on success, -1 on failure
 */
int send_operation(int sock, const char *operation);

/**
 * @brief Send a string with length prefix
 * @param sock Socket file descriptor
 * @param str String to send
 * @return int 0 on success, -1 on failure
 */
int send_string(int sock, const char *str);

/**
 * @brief Receive a string with length prefix
 * @param sock Socket file descriptor
 * @param buffer Buffer to store received string
 * @param max_len Maximum buffer size
 * @return int Length of string received, -1 on failure
 */
int recv_string(int sock, char *buffer, size_t max_len);

/**
 * @brief Send file data from an open FILE pointer
 * @param sock Socket file descriptor
 * @param fp Open file pointer (must be positioned at start of data)
 * @param file_size Size of data to send
 * @return long Number of bytes sent, -1 on failure
 */
long send_file_data(int sock, FILE *fp, long file_size);

/**
 * @brief Receive file data into an open FILE pointer
 * @param sock Socket file descriptor
 * @param fp Open file pointer for writing
 * @param file_size Size of data to receive
 * @return long Number of bytes received, -1 on failure
 */
long recv_file_data(int sock, FILE *fp, long file_size);

/**
 * @brief Send a file (opens file, sends size + data)
 * @param sock Socket file descriptor
 * @param filename Name of the file to send
 * @return long Number of bytes sent, -1 on failure
 */
long send_file(int sock, const char *filename);

/**
 * @brief Receive a file (receives size + data, creates file)
 * @param sock Socket file descriptor
 * @param filename Name of the file to receive
 * @return long Number of bytes received, -1 on failure
 */
long receive_file(int sock, const char *filename);

/**
 * @brief Receive a response
 * @param sock Socket file descriptor
 * @param buffer Buffer to store the received response
 * @param buffer_size Size of the buffer
 * @return int Number of bytes received, -1 on failure
 */
int receive_response(int sock, char *buffer, size_t buffer_size);

/**
 * @brief Create a server socket, bind it to the specified IP and port, and start listening
 * @param ip IP address to bind to (use "0.0.0.0" for all interfaces)
 * @param port Port number to bind to
 * @return int Socket file descriptor on success, -1 on failure
 */
int create_server_socket(const char *ip, int port);

#endif // NETWORK_H