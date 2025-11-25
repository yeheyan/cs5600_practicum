#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include "operations.h"
#include "server_handlers.h"
#include "config.h"

static int global_socket_desc = -1;
// Structure to pass to thread
typedef struct
{
  int client_sock;
  struct sockaddr_in client_addr;
} thread_args_t;

// Signal handler for graceful shutdown
void signal_handler(int signum)
{
  printf("\n\n[SIGNAL] Received signal %d (Ctrl+C)\n", signum);
  printf("[SIGNAL] Initiating graceful shutdown...\n");

  // Set server to stop
  set_server_running(0);

  // Close listening socket to unblock accept()
  if (global_socket_desc >= 0)
  {
    printf("[SIGNAL] Closing listening socket\n");
    close(global_socket_desc);
    global_socket_desc = -1;
  }

  printf("[SIGNAL] Server will shut down after current operations complete\n");
}

// Thread function to handle each client
void *handle_client(void *arg)
{
  thread_args_t *args = (thread_args_t *)arg;
  int client_sock = args->client_sock;
  struct sockaddr_in client_addr = args->client_addr;

  printf("\n[Thread %lu] Client connected from %s:%d\n",
         (unsigned long)pthread_self(),
         inet_ntoa(client_addr.sin_addr),
         ntohs(client_addr.sin_port));

  // Receive operation length
  int op_len;
  if (recv(client_sock, &op_len, sizeof(int), 0) <= 0)
  {
    printf("[Thread %lu] Failed to receive operation length\n",
           (unsigned long)pthread_self());
    close(client_sock);
    free(args);
    return NULL;
  }

  // Receive operation string
  char operation_str[10];
  memset(operation_str, 0, sizeof(operation_str));
  if (recv(client_sock, operation_str, op_len, 0) <= 0)
  {
    printf("[Thread %lu] Failed to receive operation\n",
           (unsigned long)pthread_self());
    close(client_sock);
    free(args);
    return NULL;
  }

  operation_str[op_len] = '\0';
  Operation op = parse_operation(operation_str);
  printf("[Thread %lu] Operation: %s\n",
         (unsigned long)pthread_self(), operation_to_string(op));

  // Dispatch to handlers - ALL handlers are in server_handlers.c
  switch (op)
  {
  case OP_WRITE:
    handle_write_request(client_sock);
    break;

  case OP_GET:
    handle_get_request(client_sock);
    break;

  case OP_RM:
    handle_rm_request(client_sock);
    break;

  case OP_LS:
    handle_ls_request(client_sock);
    break;

  case OP_STOP:
    handle_stop_request();
    break;

  case OP_UNKNOWN:
  default:
    printf("[Thread %lu] Unknown operation: %s\n",
           (unsigned long)pthread_self(), operation_str);
    break;
  }

  close(client_sock);
  printf("[Thread %lu] Client disconnected\n", (unsigned long)pthread_self());
  free(args);

  return NULL;
}

int main(void)
{
  int socket_desc;
  socklen_t client_size;
  struct sockaddr_in server_addr, client_addr;

  // Register signal handlers
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGINT, &sa, NULL) == -1)
  {
    perror("Failed to register SIGINT handler");
  }

  if (sigaction(SIGTERM, &sa, NULL) == -1)
  {
    perror("Failed to register SIGTERM handler");
  }

  printf("Signal handlers registered (Ctrl+C for graceful shutdown)\n");

  // Create socket
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc < 0)
  {
    printf("Error while creating socket\n");
    return -1;
  }
  printf("Socket created successfully\n");

  // Create storage root directory
  if (mkdir(STORAGE_ROOT, 0755) != 0 && errno != EEXIST)
  {
    perror("Failed to create storage root");
    return -1;
  }
  printf("Storage root: %s\n", STORAGE_ROOT);

  // Allow port reuse
  int opt = 1;
  setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // Set port and IP
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  // Bind
  if (bind(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    printf("Couldn't bind to the port\n");
    return -1;
  }
  printf("Done with binding\n");

  // Listen
  if (listen(socket_desc, 5) < 0)
  {
    printf("Error while listening\n");
    close(socket_desc);
    return -1;
  }
  printf("\nListening for incoming connections on port %d.....\n", SERVER_PORT);
  // Set socket timeout so accept() doesn't block forever
  struct timeval timeout;
  timeout.tv_sec = 1; // 1 second timeout
  timeout.tv_usec = 0;

  if (setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
  {
    perror("Failed to set socket timeout");
  }
  // Main server loop
  // Main server loop with select()
  while (is_server_running())
  {
    // Use select to wait for connection with timeout
    fd_set read_fds;

    FD_ZERO(&read_fds);
    FD_SET(socket_desc, &read_fds);


    // Wait for socket to be readable (incoming connection) or timeout
    int activity = select(socket_desc + 1, &read_fds, NULL, NULL, &timeout);

    if (activity < 0)
    {
      // Check if interrupted by signal
      if (errno == EINTR)
      {
        printf("[INFO] Select interrupted by signal\n");
        continue;
      }

      if (!is_server_running())
      {
        break;
      }
      perror("Select error");
      continue;
    }

    if (activity == 0)
    {
      // Timeout - no incoming connection
      // Loop will check is_server_running() again
      continue;
    }
    // Check if we're still running (might have been stopped by signal)
    if (!is_server_running())
    {
      break;
    }

    // Socket is readable - there's an incoming connection
    client_size = sizeof(client_addr);
    int client_sock = accept(socket_desc, (struct sockaddr *)&client_addr, &client_size);

    if (client_sock < 0)
    {
      // Check if we're shutting down
      if (!is_server_running())
      {
        printf("[INFO] Accept interrupted - server shutting down\n");
        break;
      }

      // Check if interrupted by signal
      if (errno == EINTR)
      {
        continue;
      }

      perror("Accept failed");
      continue;
    }

    // Create thread arguments
    thread_args_t *args = malloc(sizeof(thread_args_t));
    if (!args)
    {
      perror("Failed to allocate memory for thread args");
      close(client_sock);
      continue;
    }

    args->client_sock = client_sock;
    args->client_addr = client_addr;

    // Create thread to handle client
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, args) != 0)
    {
      perror("Failed to create thread");
      close(client_sock);
      free(args);
      continue;
    }

    // Detach thread so it cleans up automatically
    pthread_detach(thread_id);
  }

  // Clean shutdown
  printf("Server shutting down gracefully...\n");

  // Close listening socket if not already closed
  if (global_socket_desc >= 0)
  {
    close(global_socket_desc);
    global_socket_desc = -1;
  }

  printf("Listening socket closed\n");
  printf("Active connections will complete\n");
  printf("Storage root preserved: %s\n", STORAGE_ROOT);
  printf("Server stopped successfully\n");

  return 0;
}