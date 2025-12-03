/*
 * version_manager.h, Yehen Yan, CS5600 Practicum II
 * Version management declarations
 * Last modified: Dec 2025
 */

#ifndef VERSION_MANAGER_H
#define VERSION_MANAGER_H

#include <pthread.h>
#include "config.h"

// Expose version mutexes for use in server_handlers
extern pthread_mutex_t version_mutexes[HASH_SIZE];

/**
 * @brief Backup existing file by renaming it with a timestamped version suffix
 *
 * @param filename  Path to the file to back up
 */
void backup_file(const char *filename);

/**
 * @brief Extract timestamp from versioned filename
 *
 * @param version_filename  Versioned filename
 * @return time_t  Extracted timestamp, or 0 if not found
 */
time_t extract_version_timestamp(const char *version_filename);

/**
 * @brief Resolve the path of a specific version of a file
 *
 * @param full_path       Full path to the main file
 * @param version_number  Version number to retrieve (1 = newest)
 * @param version_path    Buffer to store the resolved version path
 * @param size            Size of the version_path buffer
 * @return int 0 on success, -1 on failure
 */
unsigned int hash_string(const char *str);

/**
 * @brief Resolve the path of a specific version of a file
 *
 * @param full_path       Full path to the main file
 * @param version_number  Version number to retrieve (1 = newest)
 * @param version_path    Buffer to store the resolved version path
 * @param size            Size of the version_path buffer
 * @return int 0 on success, -1 on failure
 */
int resolve_version_path(const char *full_path, int version_number, char *version_path, size_t size);

#endif // VERSION_MANAGER_H