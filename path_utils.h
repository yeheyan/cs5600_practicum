/*
 * path_utils.h, Yehen Yan, CS5600 Practicum II
 * Path validation and directory management function declarations
 * Last modified: Dec 2025
 */

#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <stddef.h>

/**
 * @brief Validate a given path for security and correctness
 *
 * @param path The path to validate
 * @return int 0 if valid, -1 if invalid
 */
int validate_path(const char *path);

/**
 * @brief Build the full storage path from a relative path
 *
 * @param relative_path The relative path
 * @param full_path Buffer to store the full path
 * @param size Size of the buffer
 */
void build_storage_path(const char *relative_path, char *full_path, size_t size);

/**
 * @brief Create a directories object
 *
 * @param path The directory path to create
 * @return int 0 on success, -1 on failure
 */
int create_directories(const char *path);

/**
 * @brief Thread-safe version of create_directories
 *
 * @param path The directory path to create
 * @return int 0 on success, -1 on failure
 */
int create_directories_safe(const char *path);

#endif // PATH_UTILS_H