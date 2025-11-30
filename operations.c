#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "operations.h"
#include "network.h"
#include "config.h"

Operation parse_operation(const char *op_str)
{
    if (strcmp(op_str, "WRITE") == 0)
        return OP_WRITE;
    if (strcmp(op_str, "GET") == 0)
        return OP_GET;
    if (strcmp(op_str, "GETVERSION") == 0)
        return OP_GETVERSION;
    if (strcmp(op_str, "RM") == 0)
        return OP_RM;
    if (strcmp(op_str, "LS") == 0)
        return OP_LS;
    if (strcmp(op_str, "STOP") == 0)
        return OP_STOP;
    return OP_UNKNOWN;
}

const char *operation_to_string(Operation op)
{
    switch (op)
    {
    case OP_WRITE:
        return "WRITE";
    case OP_GET:
        return "GET";
    case OP_GETVERSION:
        return "GETVERSION";
    case OP_RM:
        return "RM";
    case OP_LS:
        return "LS";
    case OP_STOP:
        return "STOP";
    default:
        return "UNKNOWN";
    }
}

void write_file(char *local_file, char *remote_file)
{
    char remote_path[256];

    // Determine remote path to send
    if (remote_file == NULL)
    {
        // Extract just the filename
        char *temp = strdup(local_file);
        char *filename = basename(temp);
        strncpy(remote_path, filename, sizeof(remote_path) - 1);
        remote_path[sizeof(remote_path) - 1] = '\0';
        free(temp);
    }
    else if (remote_file[strlen(remote_file) - 1] == '/')
    {
        // Trailing slash - append local filename
        char *temp = strdup(local_file);
        char *filename = basename(temp);
        snprintf(remote_path, sizeof(remote_path), "%s%s", remote_file, filename);
        free(temp);
    }
    else
    {
        // Use as-is
        strncpy(remote_path, remote_file, sizeof(remote_path) - 1);
        remote_path[sizeof(remote_path) - 1] = '\0';
    }

    printf("Writing '%s' to %s:%d as '%s'\n",
           local_file, SERVER_IP, SERVER_PORT, remote_path);

    int sock = connect_to_server(SERVER_IP, SERVER_PORT);
    if (sock < 0)
        return;

    if (send_operation(sock, "WRITE") < 0)
    {
        close(sock);
        return;
    }

    if (send_string(sock, remote_path) < 0)
    {
        close(sock);
        return;
    }

    long bytes_sent = send_file(sock, local_file);
    if (bytes_sent >= 0)
    {
        printf("Sent %ld bytes to server\n", bytes_sent);
    }

    close(sock);
}

void get_file(char *remote_file, char *local_file)
{
    char local_path[256];

    // Determine what local path to use
    if (local_file == NULL)
    {
        // Extract just the filename from remote path
        char *temp = strdup(remote_file);
        char *filename = basename(temp);
        strncpy(local_path, filename, sizeof(local_path) - 1);
        local_path[sizeof(local_path) - 1] = '\0';
        free(temp);
    }
    else
    {
        // Use the provided local path
        strncpy(local_path, local_file, sizeof(local_path) - 1);
        local_path[sizeof(local_path) - 1] = '\0';
    }

    printf("Downloading '%s' from %s:%d to '%s'\n",
           remote_file, SERVER_IP, SERVER_PORT, local_path);

    int sock = connect_to_server(SERVER_IP, SERVER_PORT);
    if (sock < 0)
        return;

    if (send_operation(sock, "GET") < 0)
    {
        close(sock);
        return;
    }

    if (send_string(sock, remote_file) < 0)
    {
        close(sock);
        return;
    }

    long bytes_received = receive_file(sock, local_path);
    if (bytes_received >= 0)
    {
        printf("Received %ld bytes, saved to '%s'\n", bytes_received, local_path);
    }

    close(sock);
}

void get_version(char *remote_file, int version_number, char *local_file)
{
    char local_path[256];
    char request[512];

    // Build request: "filename:version_number"
    snprintf(request, sizeof(request), "%s:%d", remote_file, version_number);

    // Determine local filename
    if (local_file == NULL)
    {
        char *temp = strdup(remote_file);
        char *filename = basename(temp);
        snprintf(local_path, sizeof(local_path), "%s.v%d", filename, version_number);
        free(temp);
        local_file = local_path;
    }

    printf("Requesting version %d of '%s' from %s:%d, saving to '%s'\n",
           version_number, remote_file, SERVER_IP, SERVER_PORT, local_file);

    int sock = connect_to_server(SERVER_IP, SERVER_PORT);
    if (sock < 0)
        return;

    if (send_operation(sock, "GETVERSION") < 0)
    {
        close(sock);
        return;
    }

    // Send request (filename:version)
    if (send_string(sock, request) < 0)
    {
        close(sock);
        return;
    }

    long bytes_received = receive_file(sock, local_file);
    if (bytes_received >= 0)
    {
        printf("✓ Received version %d: %ld bytes, saved to '%s'\n",
               version_number, bytes_received, local_file);
    }
    else
    {
        fprintf(stderr, "✗ Version %d not found or failed to retrieve\n", version_number);
    }

    close(sock);
}

void remove_file(char *remote_file)
{
    int sock = connect_to_server(SERVER_IP, SERVER_PORT);
    if (sock < 0)
        return;

    if (send_operation(sock, "RM") < 0)
    {
        close(sock);
        return;
    }

    if (send_string(sock, remote_file) < 0)
    {
        close(sock);
        return;
    }

    char response[256];
    if (receive_response(sock, response, sizeof(response)) > 0)
    {
        printf("%s\n", response);
    }

    close(sock);
}

void list_directory(char *remote_dir)
{
    int sock = connect_to_server(SERVER_IP, SERVER_PORT);
    if (sock < 0)
        return;

    if (send_operation(sock, "LS") < 0)
    {
        close(sock);
        return;
    }

    if (send_string(sock, remote_dir) < 0)
    {
        close(sock);
        return;
    }

    char buffer[4096];
    int bytes;
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes] = '\0';
        printf("%s", buffer);
    }

    close(sock);
}

void stop_server(void)
{
    int sock = connect_to_server(SERVER_IP, SERVER_PORT);
    if (sock < 0)
        return;

    if (send_operation(sock, "STOP") < 0)
    {
        close(sock);
        return;
    }

    printf("Server stop signal sent\n");

    close(sock);
}