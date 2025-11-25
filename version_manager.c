#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include "version_manager.h"
#include "file_utils.h"
#include "config.h"

// Hash table of mutexes for versioning
pthread_mutex_t version_mutexes[HASH_SIZE] = {
    [0 ... HASH_SIZE - 1] = PTHREAD_MUTEX_INITIALIZER};

// Hash function
unsigned int hash_string(const char *str)
{
    unsigned int hash = 0;
    while (*str)
    {
        hash = hash * 31 + *str++;
    }
    return hash % HASH_SIZE;
}

// Backup file by renaming with version timestamp
void backup_file(const char *filename)
{
    if (!file_exists(filename))
    {
        return;
    }

    char versioned_name[512];
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Cast tv_usec to long to match format specifier
    snprintf(versioned_name, sizeof(versioned_name),
             "%s.v%ld%06ld", filename, (long)tv.tv_sec, (long)tv.tv_usec);

    printf("Backing up existing file to: %s\n", versioned_name);

    if (rename(filename, versioned_name) == 0)
    {
        printf("Previous version saved as: %s\n", versioned_name);
    }
    else
    {
        perror("Failed to create backup");
    }
}

// Thread-safe backup
void backup_file_safe(const char *filename)
{
    unsigned int hash = hash_string(filename);

    pthread_mutex_lock(&version_mutexes[hash]);
    backup_file(filename);
    pthread_mutex_unlock(&version_mutexes[hash]);
}

// Extract timestamp from version filename
time_t extract_version_timestamp(const char *version_filename)
{
    const char *v_pos = strrchr(version_filename, 'v');
    if (v_pos && *(v_pos - 1) == '.')
    {
        return (time_t)atol(v_pos + 1);
    }
    return 0;
}