/*
 * File utility functions for remote file system, Yehen Yan, CS5600 Practicum II
 * Last modified: Dec 2025
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <sys/file.h>
#include <sys/socket.h>
#include "file_utils.h"
#include "config.h"
#include "network.h"

int file_exists(const char *filename)
{
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

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
    // Iterate and delete matching versions in directory
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

time_t get_file_mtime(const char *filename)
{
    struct stat st;
    if (stat(filename, &st) == 0)
    {
        return st.st_mtime;
    }
    return 0;
}

void format_timestamp(time_t timestamp, char *buffer, size_t size)
{
    struct tm *tm_info = localtime(&timestamp);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

long send_file_with_lock(int client_sock, const char *filepath)
{
    // Open file for reading
    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        perror("Failed to open file");
        long error = -1;
        send_all(client_sock, &error, sizeof(long)); // Use send_all
        return -1;
    }

    // Lock file for reading (shared lock)
    int fd = fileno(file);
    if (flock(fd, LOCK_SH) != 0)
    {
        perror("Failed to lock file");
        fclose(file);
        long error = -1;
        send_all(client_sock, &error, sizeof(long)); // Use send_all
        return -1;
    }
    printf("[LOCKED] %s for reading\n", filepath);

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Send file size
    send_all(client_sock, &file_size, sizeof(long)); // Use send_all

    // Send file data using shared function
    long total_sent = send_file_data(client_sock, file, file_size);

    // Unlock and close
    flock(fd, LOCK_UN);
    printf("[UNLOCKED] %s\n", filepath);
    fclose(file);

    return total_sent;
}