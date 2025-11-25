#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "network.h"

#define BUFFER_SIZE 4096

// Create and connect socket to server
int connect_to_server(const char *server_ip, int port)
{
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(sock);
        return -1;
    }

    return sock;
}

// Send operation type
int send_operation(int sock, const char *operation)
{
    int op_len = strlen(operation);

    // Send operation length FIRST
    if (send(sock, &op_len, sizeof(int), 0) < 0)
    {
        perror("Failed to send operation length");
        return -1;
    }

    // THEN send operation string
    if (send(sock, operation, op_len, 0) < 0)
    {
        perror("Failed to send operation");
        return -1;
    }

    return 0;
}

// Send a string with its length prefix
int send_string(int sock, const char *str)
{
    int len = strlen(str);

    // Send length first
    if (send(sock, &len, sizeof(int), 0) < 0)
    {
        perror("Failed to send string length");
        return -1;
    }

    // Send string data
    if (send(sock, str, len, 0) < 0)
    {
        perror("Failed to send string");
        return -1;
    }

    return 0;
}

// Send file contents
long send_file(int sock, const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        perror("Failed to open file");
        return -1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Send file size
    if (send(sock, &file_size, sizeof(long), 0) < 0)
    {
        perror("Failed to send file size");
        fclose(file);
        return -1;
    }

    // Send file data
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    long total_sent = 0;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        if (send(sock, buffer, bytes_read, 0) < 0)
        {
            perror("Failed to send file data");
            fclose(file);
            return -1;
        }
        total_sent += bytes_read;
    }

    fclose(file);
    return total_sent;
}

// Receive file contents
long receive_file(int sock, const char *filename)
{
    // Receive file size
    long file_size;
    if (recv(sock, &file_size, sizeof(long), 0) <= 0)
    {
        perror("Failed to receive file size");
        return -1;
    }

    if (file_size < 0)
    {
        fprintf(stderr, "File not found on server\n");
        return -1;
    }

    // Open local file
    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        perror("Failed to create local file");
        return -1;
    }

    // Receive file data
    char buffer[BUFFER_SIZE];
    long total_received = 0;
    ssize_t bytes_received;

    while (total_received < file_size)
    {
        bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
            break;

        fwrite(buffer, 1, bytes_received, file);
        total_received += bytes_received;
    }

    fclose(file);
    return total_received;
}

// Receive a response message
int receive_response(int sock, char *buffer, size_t buffer_size)
{
    int bytes = recv(sock, buffer, buffer_size - 1, 0);
    if (bytes > 0)
    {
        buffer[bytes] = '\0';
        return bytes;
    }
    return -1;
}