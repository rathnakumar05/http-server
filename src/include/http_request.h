#ifndef _HTTP_REQUEST_H
#define _HTTP_REQUEST_H

#include "http_server.h"

int handle_recv (int s, char *buffer, int buffer_size, int *response_code);
int parse_request_line (char *buffer, http_request_line_t * request_line);
void handle_request(client_connection_t *client_connection);

#endif