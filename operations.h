#ifndef OPERATIONS_H
#define OPERATIONS_H

// Operation types
typedef enum
{
    OP_WRITE,
    OP_GET,
    OP_RM,
    OP_LS,
    OP_STOP,
    OP_UNKNOWN
} Operation;

// Function to convert string to operation enum
Operation parse_operation(const char *op_str);

// Function to convert operation enum to string
const char *operation_to_string(Operation op);

/**
 * @brief Write a local file to the remote server
 *
 * @param local_file Path to the local file to be sent
 * @param remote_file Path where the file will be stored on the server, or NULL to use basename of local_file
 */
void write_file(char *local_file, char *remote_file);

/**
 * @brief Get a remote file from the server and save it locally
 *
 * @param remote_file Path to the remote file to be read
 * @param local_file Path where the file will be saved locally, or NULL to use current directory with same basename
 */
void get_file(char *remote_file, char *local_file);

/**
 * @brief Delete a remote file from the server
 *
 * @param remote_file Path to the remote file to be deleted
 */
void remove_file(char *remote_file);

/**
 * @brief gets all versioning information about a file
 *
 * @param remote_file Path to the remote file to be listed
 */
void list_directory(char *remote_file);

/**
 * @brief Send a STOP command to the server to terminate it
 */
void stop_server();

#endif