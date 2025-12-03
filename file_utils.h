/*
 * file_utils.h, Yehen Yan, CS5600 Practicum II
 * File utility functions for remote file system
 * Last modified: Dec 2025
 */
#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <time.h>

/**
 * @brief check if a file exists
 *
 * @param filename path to the file
 * @return int 1 if exists, 0 otherwise
 */
int file_exists(const char *filename);

/**
 * @brief delete a single file
 * @param filepath path to the file
 * @return int 1 if deleted, 0 if not found, -1 if error
 */
int delete_single_file(const char *filepath);

/**
 * @brief delete all versions of a file
 * @param full_path full path to the main file
 * @param deleted pointer to count of deleted files
 * @param failed pointer to count of failed deletions
 * @return int 1 on success
 */
int delete_file_versions(const char *full_path, int *deleted, int *failed);

/**
 * @brief get file modification time
 *
 * @param filename path to the file
 * @return time_t modification time, 0 if error
 */
time_t get_file_mtime(const char *filename);

/**
 * @brief format timestamp as readable string
 *
 * @param timestamp time_t timestamp
 * @param buffer output buffer
 * @param size size of output buffer
 */
void format_timestamp(time_t timestamp, char *buffer, size_t size);

/**
 * @brief send a file over socket with locking
 *
 * @param client_sock socket descriptor
 * @param filepath path to the file
 * @return long number of bytes sent, -1 on error
 */
long send_file_with_lock(int client_sock, const char *filepath);

#endif // FILE_UTILS_H