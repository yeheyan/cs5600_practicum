#ifndef SERVER_HANDLERS_H
#define SERVER_HANDLERS_H

// Handler functions - ONLY these
void handle_write_request(int client_sock);
void handle_get_request(int client_sock);
void handle_getversion_request(int client_sock);
void handle_rm_request(int client_sock);
void handle_ls_request(int client_sock);
void handle_stop_request(void);

// Server control
void set_server_running(int value);
int is_server_running(void);

#endif