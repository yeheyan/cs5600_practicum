CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

CLIENT_OBJS = client.o operations.o network.o
SERVER_OBJS = server.o server_handlers.o file_utils.o path_utils.o version_manager.o operations.o network.o

all: rfs server

rfs: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o rfs

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o server -pthread

# Client
client.o: client.c operations.h config.h
	$(CC) $(CFLAGS) -c client.c

operations.o: operations.c operations.h network.h config.h
	$(CC) $(CFLAGS) -c operations.c

network.o: network.c network.h
	$(CC) $(CFLAGS) -c network.c

# Server
server.o: server.c operations.h server_handlers.h config.h
	$(CC) $(CFLAGS) -c server.c

server_handlers.o: server_handlers.c server_handlers.h file_utils.h path_utils.h version_manager.h operations.h
	$(CC) $(CFLAGS) -c server_handlers.c

file_utils.o: file_utils.c file_utils.h
	$(CC) $(CFLAGS) -c file_utils.c

path_utils.o: path_utils.c path_utils.h config.h
	$(CC) $(CFLAGS) -c path_utils.c

version_manager.o: version_manager.c version_manager.h file_utils.h
	$(CC) $(CFLAGS) -c version_manager.c

clean:
	rm -f *.o rfs server

.PHONY: all clean