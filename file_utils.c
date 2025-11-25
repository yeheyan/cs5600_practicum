#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include "file_utils.h"

// Check if file exists
int file_exists(const char *filename)
{
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

// Delete a single file
int delete_single_file(const char *filepath)
{
    if (file_exists(filepath))
    {
        if (remove(filepath) == 0)
        {
            printf("Deleted: %s\n", filepath);
            return 1; // Success
        }
        else
        {
            perror("Failed to delete");
            return -1; // Failed
        }
    }
    return 0; // Didn't exist
}

// Delete all versions of a file
int delete_file_versions(const char *full_path, int *deleted, int *failed)
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

    DIR *dir = opendir(dir_path);
    if (!dir)
    {
        return 0;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        char full_entry_path[512];
        snprintf(full_entry_path, sizeof(full_entry_path), "%s/%s",
                 dir_path, entry->d_name);

        if (strncmp(full_entry_path, pattern, strlen(pattern)) == 0)
        {
            printf("Deleting version: %s\n", full_entry_path);
            if (remove(full_entry_path) == 0)
            {
                (*deleted)++;
            }
            else
            {
                perror("Failed to delete version");
                (*failed)++;
            }
        }
    }

    closedir(dir);
    return 1;
}

// Get file modification time
time_t get_file_mtime(const char *filename)
{
    struct stat st;
    if (stat(filename, &st) == 0)
    {
        return st.st_mtime;
    }
    return 0;
}

// Format timestamp as readable string
void format_timestamp(time_t timestamp, char *buffer, size_t size)
{
    struct tm *tm_info = localtime(&timestamp);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}