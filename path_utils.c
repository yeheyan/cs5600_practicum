/*
 * path_utils.c, Yehen Yan, CS5600 Practicum II
 * Path validation and directory management functions
 * Last modified: Dec 2025
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include "path_utils.h"
#include "config.h"

static pthread_mutex_t dir_mutex = PTHREAD_MUTEX_INITIALIZER;

int validate_path(const char *path)
{
    // Reject absolute paths
    if (path[0] == '/')
    {
        fprintf(stderr, "Rejected: Absolute paths not allowed\n");
        return -1;
    }

    // Reject paths with '..'
    if (strstr(path, "..") != NULL)
    {
        fprintf(stderr, "Rejected: Path traversal not allowed\n");
        return -1;
    }

    // Check path depth
    int depth = 0;
    for (const char *p = path; *p; p++)
    {
        if (*p == '/')
            depth++;
    }
    if (depth > MAX_PATH_DEPTH)
    {
        fprintf(stderr, "Rejected: Path too deep (max %d levels)\n", MAX_PATH_DEPTH);
        return -1;
    }

    return 0;
}

void build_storage_path(const char *relative_path, char *full_path, size_t size)
{
    snprintf(full_path, size, "%s/%s", STORAGE_ROOT, relative_path);
}

int create_directories(const char *path)
{
    char tmp[512];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;

    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
            {
                perror("mkdir failed");
                return -1;
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
    {
        perror("mkdir failed");
        return -1;
    }

    return 0;
}

int create_directories_safe(const char *path)
{
    pthread_mutex_lock(&dir_mutex);
    int result = create_directories(path);
    pthread_mutex_unlock(&dir_mutex);
    return result;
}