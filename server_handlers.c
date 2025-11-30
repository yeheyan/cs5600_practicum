#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include "server_handlers.h"
#include "file_utils.h"
#include "path_utils.h"
#include "version_manager.h"
#include "operations.h"
#include "config.h"

// Server state
static volatile int server_running = 1;
static pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
// Server control functions
void set_server_running(int value)
{
    pthread_mutex_lock(&server_mutex);
    server_running = value;
    pthread_mutex_unlock(&server_mutex);
}

int is_server_running(void)
{
    int running;
    pthread_mutex_lock(&server_mutex);
    running = server_running;
    pthread_mutex_unlock(&server_mutex);
    return running;
}

// WRITE handler
void handle_write_request(int client_sock)
{
    char filename[256];
    int filename_len;
    long file_size;
    char buffer[BUFFER_SIZE];

    // Receive filename
    if (recv(client_sock, &filename_len, sizeof(int), 0) <= 0 ||
        recv(client_sock, filename, filename_len, 0) <= 0)
    {
        perror("Failed to receive filename");
        return;
    }
    filename[filename_len] = '\0';
    printf("Received path from client: %s\n", filename);

    // Validate and build path
    if (validate_path(filename) != 0)
    {
        printf("Rejected invalid path: %s\n", filename);
        return;
    }

    char full_path[512];
    build_storage_path(filename, full_path, sizeof(full_path));
    printf("Saving to: %s\n", full_path);

    // Receive file size
    if (recv(client_sock, &file_size, sizeof(long), 0) <= 0)
    {
        perror("Failed to receive file size");
        return;
    }
    printf("File size: %ld bytes (%.2f MB)\n", file_size, file_size / (1024.0 * 1024.0));

    // Create directories if needed
    char *last_slash = strrchr(full_path, '/');
    if (last_slash)
    {
        char dir_path[512];
        size_t dir_len = last_slash - full_path;
        strncpy(dir_path, full_path, dir_len);
        dir_path[dir_len] = '\0';

        if (create_directories_safe(dir_path) != 0)
        {
            fprintf(stderr, "Failed to create directory structure\n");
            return;
        }
    }

    // LOCK FOR ENTIRE WRITE OPERATION (backup + write)
    unsigned int hash = hash_string(full_path);
    pthread_mutex_lock(&version_mutexes[hash]);
    printf("[WRITE MUTEX LOCKED] for %s\n", full_path);

    // Backup existing file
    backup_file(full_path);

    // Open file for writing
    FILE *file = fopen(full_path, "wb");
    if (!file)
    {
        perror("Failed to create file");
        pthread_mutex_unlock(&version_mutexes[hash]);
        printf("[WRITE MUTEX UNLOCKED] for %s\n", full_path);
        return;
    }

    // File-level lock (for coordination with readers)
    int fd = fileno(file);
    if (flock(fd, LOCK_EX) != 0)
    {
        perror("Failed to lock file");
        fclose(file);
        pthread_mutex_unlock(&version_mutexes[hash]);
        printf("[WRITE MUTEX UNLOCKED] for %s\n", full_path);
        return;
    }
    printf("[FILE LOCKED] %s for writing\n", full_path);

    // Receive file data
    long total_received = 0;
    ssize_t bytes_received;
    int write_error = 0;

    while (total_received < file_size)
    {
        bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
        {
            if (bytes_received < 0)
            {
                perror("recv failed");
            }
            break;
        }

        size_t written = fwrite(buffer, 1, bytes_received, file);

        // CHECK FOR WRITE ERROR (disk full, etc.)
        if (written != (size_t)bytes_received)
        {
            fprintf(stderr, "[ERROR] Write failed - storage may be full\n");
            perror("fwrite");
            write_error = 1;
            break;
        }

        total_received += bytes_received;
    }

    // Check for file stream errors
    if (!write_error && ferror(file))
    {
        fprintf(stderr, "[ERROR] File write error occurred\n");
        perror("File error");
        write_error = 1;
    }

    // Check if we received complete file
    if (!write_error && total_received != file_size)
    {
        fprintf(stderr, "[ERROR] Incomplete file - expected %ld, got %ld bytes\n",
                file_size, total_received);
        write_error = 1;
    }

    // Unlock and close file
    flock(fd, LOCK_UN);
    printf("[FILE UNLOCKED] %s\n", full_path);
    fclose(file);

    // Handle write errors
    if (write_error)
    {
        remove(full_path);
        printf("Partial/corrupted file deleted\n");
        pthread_mutex_unlock(&version_mutexes[hash]);
        printf("[WRITE MUTEX UNLOCKED] for %s\n", full_path);
        return;
    }

    // UNLOCK WRITE MUTEX
    pthread_mutex_unlock(&version_mutexes[hash]);
    printf("[WRITE MUTEX UNLOCKED] for %s\n", full_path);

    printf("File saved successfully: %ld bytes to %s\n", total_received, full_path);
}

// GET handler
// Regular GET - just get current file
void handle_get_request(int client_sock)
{
    char filename[256];
    int filename_len;

    // Receive filename
    if (recv(client_sock, &filename_len, sizeof(int), 0) <= 0 ||
        recv(client_sock, filename, filename_len, 0) <= 0)
    {
        perror("Failed to receive filename");
        return;
    }
    filename[filename_len] = '\0';

    printf("GET request for: %s\n", filename);

    // Validate path
    if (validate_path(filename) != 0)
    {
        long error = -1;
        send(client_sock, &error, sizeof(long), 0);
        return;
    }

    // Build full storage path
    char full_path[512];
    build_storage_path(filename, full_path, sizeof(full_path));
    printf("Reading from: %s\n", full_path);

    // Use shared function to send file
    long bytes_sent = send_file_with_lock(client_sock, full_path);

    if (bytes_sent > 0)
    {
        printf("Sent file: %ld bytes\n", bytes_sent);
    }
    else
    {
        printf("Failed to send file\n");
    }
}

// GETVERSION - get specific version
void handle_getversion_request(int client_sock)
{
    char request[512];
    int request_len;

    // Receive request (format: "filename:version_number")
    if (recv(client_sock, &request_len, sizeof(int), 0) <= 0 ||
        recv(client_sock, request, request_len, 0) <= 0)
    {
        perror("Failed to receive version request");
        return;
    }
    request[request_len] = '\0';

    // Parse request
    char *colon = strchr(request, ':');
    if (!colon)
    {
        fprintf(stderr, "Invalid GETVERSION format\n");
        long error = -1;
        send(client_sock, &error, sizeof(long), 0);
        return;
    }

    *colon = '\0';
    char *filename = request;
    int version_number = atoi(colon + 1);

    printf("GETVERSION request: %s, version %d\n", filename, version_number);

    // Validate path
    if (validate_path(filename) != 0)
    {
        long error = -1;
        send(client_sock, &error, sizeof(long), 0);
        return;
    }

    // Build full path and resolve version
    char full_path[512];
    build_storage_path(filename, full_path, sizeof(full_path));

    char version_path[512];
    if (resolve_version_path(full_path, version_number, version_path, sizeof(version_path)) != 0)
    {
        fprintf(stderr, "Version %d not found\n", version_number);
        long error = -1;
        send(client_sock, &error, sizeof(long), 0);
        return;
    }

    printf("Resolved to: %s\n", version_path);

    // Use shared function to send file (REUSED!)
    long bytes_sent = send_file_with_lock(client_sock, version_path);

    if (bytes_sent > 0)
    {
        printf("Sent version %d: %ld bytes\n", version_number, bytes_sent);
    }
    else
    {
        printf("Failed to send version\n");
    }
}

// RM handler - simplified version
void handle_rm_request(int client_sock)
{
    char filename[256];
    int filename_len;
    char response[1024];

    // Receive filename
    if (recv(client_sock, &filename_len, sizeof(int), 0) <= 0 ||
        recv(client_sock, filename, filename_len, 0) <= 0)
    {
        perror("Failed to receive filename");
        return;
    }
    filename[filename_len] = '\0';

    printf("Delete request for: %s\n", filename);

    // Validate and build path
    if (validate_path(filename) != 0)
    {
        snprintf(response, sizeof(response), "Invalid path: %s\n", filename);
        send(client_sock, response, strlen(response), 0);
        return;
    }

    char full_path[512];
    build_storage_path(filename, full_path, sizeof(full_path));

    // Lock for deletion
    unsigned int hash = hash_string(full_path);
    pthread_mutex_lock(&version_mutexes[hash]);

    // Delete main file and versions
    int deleted_count = 0;
    int failed_count = 0;

    int result = delete_single_file(full_path);
    if (result > 0)
        deleted_count++;
    else if (result < 0)
        failed_count++;

    delete_file_versions(full_path, &deleted_count, &failed_count);

    pthread_mutex_unlock(&version_mutexes[hash]);

    // Send response
    if (deleted_count > 0)
    {
        snprintf(response, sizeof(response),
                 "Successfully deleted %d file(s) (main file + %d version(s))\n",
                 deleted_count, deleted_count - 1);
    }
    else
    {
        snprintf(response, sizeof(response), "File not found: %s\n", filename);
    }

    if (failed_count > 0)
    {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "Warning: Failed to delete %d file(s)\n", failed_count);
        strncat(response, error_msg, sizeof(response) - strlen(response) - 1);
    }

    send(client_sock, response, strlen(response), 0);
}

void handle_ls_request(int client_sock)
{
    char path[256];
    int path_len;
    char buffer[BUFFER_SIZE];

    // Receive path length
    if (recv(client_sock, &path_len, sizeof(int), 0) <= 0)
    {
        perror("Failed to receive path length");
        return;
    }

    // Receive path
    if (recv(client_sock, path, path_len, 0) <= 0)
    {
        perror("Failed to receive path");
        return;
    }
    path[path_len] = '\0';

    printf("LS request for: %s\n", path);

    // Validate path
    if (validate_path(path) != 0)
    {
        snprintf(buffer, sizeof(buffer), "Invalid path: %s\n", path);
        send(client_sock, buffer, strlen(buffer), 0);
        return;
    }

    // Build full storage path
    char full_path[512];
    build_storage_path(path, full_path, sizeof(full_path));

    printf("Listing: %s\n", full_path);

    struct stat st;

    // Check if path is a directory or file
    if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode))
    {
        // It's a directory - list all files
        DIR *dir = opendir(full_path);
        if (!dir)
        {
            snprintf(buffer, sizeof(buffer), "Failed to open directory: %s\n", path);
            send(client_sock, buffer, strlen(buffer), 0);
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            // Skip . and ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            // Get full path for the entry
            char entry_full_path[512];
            snprintf(entry_full_path, sizeof(entry_full_path), "%s/%s",
                     full_path, entry->d_name);

            // Get file stats
            struct stat file_stat;
            if (stat(entry_full_path, &file_stat) == 0)
            {
                char time_str[64];
                format_timestamp(file_stat.st_mtime, time_str, sizeof(time_str));

                snprintf(buffer, sizeof(buffer), "%s  %10lld bytes  %s\n",
                         entry->d_name, (long long)file_stat.st_size, time_str);
            }
            else
            {
                snprintf(buffer, sizeof(buffer), "%s\n", entry->d_name);
            }

            send(client_sock, buffer, strlen(buffer), 0);
        }

        closedir(dir);
    }
    else if (file_exists(full_path))
    {
        // It's a file - list file and all its versions
        time_t mtime = get_file_mtime(full_path);
        char time_str[64];
        format_timestamp(mtime, time_str, sizeof(time_str));

        snprintf(buffer, sizeof(buffer),
                 "[CURRENT] %s\n"
                 "  Size: %lld bytes\n"
                 "  Last Modified: %s\n\n",
                 path, (long long)st.st_size, time_str);
        send(client_sock, buffer, strlen(buffer), 0);

        // Find and list versions
        char pattern[512];
        snprintf(pattern, sizeof(pattern), "%s.v", full_path);

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
            return;
        }

        typedef struct
        {
            char filename[256];
            time_t version_timestamp;
            time_t file_mtime;
            long size;
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
                struct stat version_stat;
                if (stat(full_entry_path, &version_stat) == 0)
                {
                    strncpy(versions[version_count].filename, full_entry_path,
                            sizeof(versions[version_count].filename) - 1);

                    versions[version_count].version_timestamp =
                        extract_version_timestamp(full_entry_path);

                    versions[version_count].file_mtime = version_stat.st_mtime;
                    versions[version_count].size = version_stat.st_size;
                    version_count++;
                }
            }
        }

        closedir(dir);

        // Sort by version timestamp (newest first)
        for (int i = 0; i < version_count - 1; i++)
        {
            for (int j = i + 1; j < version_count; j++)
            {
                if (versions[j].version_timestamp > versions[i].version_timestamp)
                {
                    VersionInfo temp = versions[i];
                    versions[i] = versions[j];
                    versions[j] = temp;
                }
            }
        }

        // Display versions
        for (int i = 0; i < version_count; i++)
        {
            char written_time[64];

            if (versions[i].version_timestamp > 0)
            {
                format_timestamp(versions[i].version_timestamp, written_time,
                                 sizeof(written_time));
            }
            else
            {
                strcpy(written_time, "Unknown");
            }

            snprintf(buffer, sizeof(buffer),
                     "[VERSION %d] %s\n"
                     "  Size: %lld bytes\n"
                     "  Written: %s\n\n",
                     version_count - i,
                     versions[i].filename,
                     (long long)versions[i].size,
                     written_time);
            send(client_sock, buffer, strlen(buffer), 0);
        }

        if (version_count == 0)
        {
            snprintf(buffer, sizeof(buffer), "(No previous versions)\n");
            send(client_sock, buffer, strlen(buffer), 0);
        }
        else
        {
            snprintf(buffer, sizeof(buffer),
                     "Total: 1 current + %d version(s)\n", version_count);
            send(client_sock, buffer, strlen(buffer), 0);
        }
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "Path not found: %s\n", path);
        send(client_sock, buffer, strlen(buffer), 0);
    }
}

void handle_stop_request(void)
{
    printf("STOP command received. Shutting down server...\n");
    set_server_running(0);
}