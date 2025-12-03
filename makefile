# Makefile for Remote File System (RFS)
# CS5600 Practicum II

CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -std=c99
LDFLAGS = -pthread

# Client executable
CLIENT = rfs
CLIENT_OBJS = client.o operations.o network.o

# Server executable
SERVER = server
SERVER_OBJS = server.o server_handlers.o operations.o network.o file_utils.o version_manager.o path_utils.o

# Default target: build both client and server
all: $(CLIENT) $(SERVER)

# Build client
$(CLIENT): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o $(CLIENT) $(LDFLAGS)

# Build server
$(SERVER): $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $(SERVER) $(LDFLAGS)

# Compile client sources
client.o: client.c operations.h config.h
	$(CC) $(CFLAGS) -c client.c

# Compile server sources
server.o: server.c server_handlers.h operations.h config.h network.h
	$(CC) $(CFLAGS) -c server.c

server_handlers.o: server_handlers.c server_handlers.h operations.h file_utils.h version_manager.h path_utils.h network.h config.h
	$(CC) $(CFLAGS) -c server_handlers.c

file_utils.o: file_utils.c file_utils.h config.h network.h
	$(CC) $(CFLAGS) -c file_utils.c

version_manager.o: version_manager.c version_manager.h config.h
	$(CC) $(CFLAGS) -c version_manager.c

path_utils.o: path_utils.c path_utils.h config.h
	$(CC) $(CFLAGS) -c path_utils.c

# Compile shared modules (used by both client and server)
operations.o: operations.c operations.h network.h config.h
	$(CC) $(CFLAGS) -c operations.c

network.o: network.c network.h config.h
	$(CC) $(CFLAGS) -c network.c

# Clean build artifacts
clean:
	rm -f *.o $(CLIENT) $(SERVER)

# Clean and rebuild
rebuild: clean all

# Phony targets
.PHONY: all clean rebuild