#ifndef _SERVER_HANDLE_H
#define _SERVER_HANDLE_H

#include "server_connection.h"

void server_connect(server_connection_t *server_connection);
void start_server(server_connection_t *server_connection);
void close_server(server_connection_t *server_connection);

#endif