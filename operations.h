/*
 * operations.h, Yehen Yan, CS5600 Practicum II
 * Remote file system operation declarations
 * Last modified: Dec 2025
 */

#ifndef OPERATIONS_H
#define OPERATIONS_H

// Operation types
typedef enum
{
    OP_WRITE,
    OP_GET,
    OP_GETVERSION,
    OP_RM,
    OP_LS,
    OP_STOP,
    OP_UNKNOWN
} Operation;

/**
 * @brief Convert string to operation enum
 *
 * @param op_str
 * @return Operation
 */
Operation parse_operation(const char *op_str);

/**
 * @brief Convert operation enum to string
 *
 * @param op
 * @return const char*
 */
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
 * @brief Get a specific version of a remote file from the server and save it locally
 *
 * @param remote_file Path to the remote file to be read
 * @param version_number Version number to retrieve
 * @param local_file Path where the versioned file will be saved locally, or NULL to use current directory with modified basename
 */
void get_version(char *remote_file, int version_number, char *local_file);

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

#endif // OPERATIONS_H