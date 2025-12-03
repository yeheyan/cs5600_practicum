/*
 * server_handlers.h, Yehen Yan, CS5600 Practicum II
 * Server request handlers declarations
 * Last modified: Dec 2025
 */

#ifndef SERVER_HANDLERS_H
#define SERVER_HANDLERS_H

/**
 * @brief  Handle WRITE request from client, copying file to server
 *
 * @param client_sock socket descriptor
 */
void handle_write_request(int client_sock);

/**
 * @brief  Handle GET request from client, sending file to client
 *
 * @param client_sock socket descriptor
 */
void handle_get_request(int client_sock);

/**
 * @brief  Handle GETVERSION request from client, sending specific file version to client
 *
 * @param client_sock socket descriptor
 */
void handle_getversion_request(int client_sock);

/**
 * @brief  Handle RM request from client, deleting file and all its versions
 *
 * @param client_sock socket descriptor
 */
void handle_rm_request(int client_sock);

/**
 * @brief  Handle LS request from client, listing all versions of the file
 *
 * @param client_sock socket descriptor
 */
void handle_ls_request(int client_sock);

/**
 * @brief  Handle STOP request from client, shutting down server
 *
 */
void handle_stop_request(void);

/**
 * @brief  Set server running state
 *
 * @param value 1 to run, 0 to stop
 */
void set_server_running(int value);

/**
 * @brief  Check if server is running
 *
 * @return int 1 if running, 0 if stopped
 */
int is_server_running(void);

#endif // SERVER_HANDLERS_H