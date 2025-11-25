#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <stddef.h>

// Path validation
int validate_path(const char *path);

// Path manipulation
void build_storage_path(const char *relative_path, char *full_path, size_t size);

// Directory creation
int create_directories(const char *path);
int create_directories_safe(const char *path);

#endif