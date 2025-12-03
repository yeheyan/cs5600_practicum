/*
 * network.c, Yehen Yan, CS5600 Practicum II
 * Network communication functions for remote file system
 * Last modified: Dec 2025
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "network.h"
#include "config.h"

// ========== LOW-LEVEL RELIABLE SEND/RECV ==========

int send_all(int sock, const void *data, size_t len)
{
    size_t total_sent = 0;
    const char *ptr = (const char *)data;

    while (total_sent < len)
    {
        ssize_t sent = send(sock, ptr + total_sent, len - total_sent, 0);
        if (sent <= 0)
        {
            if (sent < 0)
            {
                perror("send_all failed");
            }
            return -1;
        }
        total_sent += sent;
    }
    return 0;
}

int recv_all(int sock, void *buffer, size_t len)
{
    size_t total_received = 0;
    char *ptr = (char *)buffer;

    while (total_received < len)
    {
        ssize_t received = recv(sock, ptr + total_received, len - total_received, 0);
        if (received <= 0)
        {
            if (received < 0)
            {
                perror("recv_all failed");
            }
            return -1;
        }
        total_received += received;
    }
    return 0;
}

// ========== STRING HELPERS ==========

int recv_string(int sock, char *buffer, size_t max_len)
{
    int str_len;

    // Receive length
    if (recv_all(sock, &str_len, sizeof(int)) < 0)
    {
        fprintf(stderr, "Failed to receive string length\n");
        return -1;
    }

    // Check bounds
    if (str_len < 0 || (size_t)str_len >= max_len)
    {
        fprintf(stderr, "Invalid string length: %d (max: %zu)\n", str_len, max_len);
        return -1;
    }

    // Receive string data
    if (recv_all(sock, buffer, str_len) < 0)
    {
        fprintf(stderr, "Failed to receive string data\n");
        return -1;
    }

    buffer[str_len] = '\0';
    return str_len;
}

// ========== FILE DATA TRANSFER ==========

long send_file_data(int sock, FILE *fp, long file_size)
{
    char buffer[BUFFER_SIZE];
    long total_sent = 0;
    size_t bytes_read;

    while (total_sent < file_size)
    {
        size_t to_read = BUFFER_SIZE;
        if (file_size - total_sent < BUFFER_SIZE)
        {
            to_read = file_size - total_sent;
        }

        bytes_read = fread(buffer, 1, to_read, fp);
        if (bytes_read == 0)
        {
            if (ferror(fp))
            {
                perror("File read error");
                return -1;
            }
            break; // EOF
        }

        if (send_all(sock, buffer, bytes_read) < 0)
        {
            fprintf(stderr, "Failed to send file data\n");
            return -1;
        }

        total_sent += bytes_read;
    }

    return total_sent;
}

long recv_file_data(int sock, FILE *fp, long file_size)
{
    char buffer[BUFFER_SIZE];
    long total_received = 0;
    ssize_t bytes_received;

    while (total_received < file_size)
    {
        size_t to_receive = BUFFER_SIZE;
        if (file_size - total_received < BUFFER_SIZE)
        {
            to_receive = file_size - total_received;
        }

        bytes_received = recv(sock, buffer, to_receive, 0);
        if (bytes_received <= 0)
        {
            if (bytes_received < 0)
            {
                perror("recv failed");
            }
            return -1;
        }

        size_t written = fwrite(buffer, 1, bytes_received, fp);
        if (written != (size_t)bytes_received)
        {
            perror("File write error");
            return -1;
        }

        total_received += bytes_received;
    }

    return total_received;
}

// ========== EXISTING FUNCTIONS (keep as-is) ==========

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

int send_operation(int sock, const char *operation)
{
    int op_len = strlen(operation);

    // Use send_all for reliability
    if (send_all(sock, &op_len, sizeof(int)) < 0)
    {
        fprintf(stderr, "Failed to send operation length\n");
        return -1;
    }

    if (send_all(sock, operation, op_len) < 0)
    {
        fprintf(stderr, "Failed to send operation\n");
        return -1;
    }

    return 0;
}

int send_string(int sock, const char *str)
{
    int len = strlen(str);

    // Use send_all for reliability
    if (send_all(sock, &len, sizeof(int)) < 0)
    {
        fprintf(stderr, "Failed to send string length\n");
        return -1;
    }

    if (send_all(sock, str, len) < 0)
    {
        fprintf(stderr, "Failed to send string\n");
        return -1;
    }

    return 0;
}

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
    if (send_all(sock, &file_size, sizeof(long)) < 0)
    {
        fprintf(stderr, "Failed to send file size\n");
        fclose(file);
        return -1;
    }

    // Send file data using shared function
    long bytes_sent = send_file_data(sock, file, file_size);
    fclose(file);

    return bytes_sent;
}

long receive_file(int sock, const char *filename)
{
    long file_size;

    // Receive file size
    if (recv_all(sock, &file_size, sizeof(long)) < 0)
    {
        fprintf(stderr, "Failed to receive file size\n");
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

    // Receive file data using shared function
    long bytes_received = recv_file_data(sock, file, file_size);
    fclose(file);

    if (bytes_received != file_size)
    {
        fprintf(stderr, "Incomplete file received: %ld/%ld bytes\n",
                bytes_received, file_size);
        remove(filename); // Clean up partial file
        return -1;
    }

    return bytes_received;
}

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

int create_server_socket(const char *ip, int port)
{
    int socket_desc;
    struct sockaddr_in server_addr;

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0)
    {
        perror("Socket creation failed");
        return -1;
    }

    // Allow port reuse (prevents "Address already in use" errors)
    int opt = 1;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
        close(socket_desc);
        return -1;
    }

    // Set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Bind to specific IP or all interfaces
    if (strcmp(ip, "0.0.0.0") == 0)
    {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        server_addr.sin_addr.s_addr = inet_addr(ip);
    }

    // Bind socket to address
    if (bind(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(socket_desc);
        return -1;
    }

    // Listen for connections (backlog of 5)
    if (listen(socket_desc, 5) < 0)
    {
        perror("Listen failed");
        close(socket_desc);
        return -1;
    }

    return socket_desc;
}