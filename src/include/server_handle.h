#ifndef _SERVER_HANDLE_H
#define _SERVER_HANDLE_H

#include "server_connection.h"
#include "client_connection.h"

void server_connect(server_connection_t *server_connection);
void handle_recv(client_connection_t *client_connection);
const char * http_reason_phrase (int code);
const char * get_mime_type (const char *path);
void http_response (client_connection_t * client_connection, char *path);
int send_all (int s, char *buff, size_t buff_len);
void handle_send (client_connection_t * client_connection);
void start_server(server_connection_t *server_connection);
void close_server(server_connection_t *server_connection);

#endif