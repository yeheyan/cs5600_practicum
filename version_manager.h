#ifndef VERSION_MANAGER_H
#define VERSION_MANAGER_H

#include <time.h>
#include <pthread.h>
#include "config.h"

// Expose version mutexes for use in server_handlers
extern pthread_mutex_t version_mutexes[HASH_SIZE];

// Version operations
void backup_file(const char *filename);
void backup_file_safe(const char *filename);
time_t extract_version_timestamp(const char *version_filename);

// Hash function for mutex selection
unsigned int hash_string(const char *str);

int resolve_version_path(const char *full_path, int version_number, char *version_path, size_t size);

#endif