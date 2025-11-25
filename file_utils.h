#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <time.h>

// File existence check
int file_exists(const char *filename);

// File deletion operations
int delete_single_file(const char *filepath);
int delete_file_versions(const char *full_path, int *deleted, int *failed);

// File metadata
time_t get_file_mtime(const char *filename);
void format_timestamp(time_t timestamp, char *buffer, size_t size);

#endif