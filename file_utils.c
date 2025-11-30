#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <sys/file.h>
#include <sys/socket.h>
#include "file_utils.h"
#include "config.h"

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

// Send a file over socket with locking
long send_file_with_lock(int client_sock, const char *filepath)
{
    char buffer[BUFFER_SIZE];

    // Open file for reading
    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        perror("Failed to open file");
        long error = -1;
        send(client_sock, &error, sizeof(long), 0);
        return -1;
    }

    // Lock file for reading (shared lock)
    int fd = fileno(file);
    if (flock(fd, LOCK_SH) != 0)
    {
        perror("Failed to lock file");
        fclose(file);
        long error = -1;
        send(client_sock, &error, sizeof(long), 0);
        return -1;
    }
    printf("[LOCKED] %s for reading\n", filepath);

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Send file size
    send(client_sock, &file_size, sizeof(long), 0);

    // Send file data
    size_t bytes_read;
    long total_sent = 0;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        send(client_sock, buffer, bytes_read, 0);
        total_sent += bytes_read;
    }

    // Unlock and close
    flock(fd, LOCK_UN);
    printf("[UNLOCKED] %s\n", filepath);
    fclose(file);

    return total_sent;
}