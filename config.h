#ifndef CONFIG_H
#define CONFIG_H

// Change these values as needed
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 2000

// Root directory for storing files on the server
#define STORAGE_ROOT "./rfs_storage"
// to prevent excessively deep paths
#define MAX_PATH_DEPTH 10

// Hash table size for tracking file versions
#define HASH_SIZE 256

// Buffer size for network operations
#define BUFFER_SIZE 8196

#endif