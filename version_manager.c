/*
 * version_manager.c, Yehen Yan, CS5600 Practicum II
 * File version management implementation
 * Last modified: Dec 2025
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <dirent.h>
#include "version_manager.h"
#include "file_utils.h"
#include "config.h"

// Hash table of mutexes for versioning
pthread_mutex_t version_mutexes[HASH_SIZE] = {
    [0 ... HASH_SIZE - 1] = PTHREAD_MUTEX_INITIALIZER};

unsigned int hash_string(const char *str)
{
    unsigned int hash = 0;
    while (*str)
    {
        hash = hash * 31 + *str++;
    }
    return hash % HASH_SIZE;
}

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

time_t extract_version_timestamp(const char *version_filename)
{
    const char *v_pos = strrchr(version_filename, 'v');
    if (!v_pos || v_pos == version_filename || *(v_pos - 1) != '.')
    {
        return 0;
    }

    char seconds_str[11];
    strncpy(seconds_str, v_pos + 1, 10);
    seconds_str[10] = '\0';

    return (time_t)atol(seconds_str);
}

int resolve_version_path(const char *full_path, int version_number, char *version_path, size_t size)
{
    char pattern[512];
    snprintf(pattern, sizeof(pattern), "%s.v", full_path);

    // Extract directory
    char dir_path[512];
    strncpy(dir_path, full_path, sizeof(dir_path) - 1);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash)
    {
        *last_slash = '\0';
    }
    else
    {
        strcpy(dir_path, ".");
    }

    // Find all versions
    DIR *dir = opendir(dir_path);
    if (!dir)
    {
        return -1;
    }

    typedef struct
    {
        char filename[512];
        time_t timestamp;
    } VersionInfo;

    VersionInfo versions[100];
    int version_count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && version_count < 100)
    {
        char full_entry_path[512];
        snprintf(full_entry_path, sizeof(full_entry_path), "%s/%s",
                 dir_path, entry->d_name);

        if (strncmp(full_entry_path, pattern, strlen(pattern)) == 0)
        {
            strncpy(versions[version_count].filename, full_entry_path,
                    sizeof(versions[version_count].filename) - 1);
            versions[version_count].timestamp = extract_version_timestamp(full_entry_path);
            version_count++;
        }
    }
    closedir(dir);

    if (version_count == 0)
    {
        return -1; // No versions found
    }

    // Sort by timestamp (newest first)
    for (int i = 0; i < version_count - 1; i++)
    {
        for (int j = i + 1; j < version_count; j++)
        {
            if (versions[j].timestamp > versions[i].timestamp)
            {
                VersionInfo temp = versions[i];
                versions[i] = versions[j];
                versions[j] = temp;
            }
        }
    }

    // Get requested version (1-indexed from newest)
    // Version 1 = newest, Version N = oldest
    int index = version_count - version_number;

    if (index < 0 || index >= version_count)
    {
        return -1; // Version number out of range
    }

    strncpy(version_path, versions[index].filename, size - 1);
    version_path[size - 1] = '\0';

    return 0; // Success
}