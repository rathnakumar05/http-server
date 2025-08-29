#ifndef _HTTP_HANDLE_H
#define _HTTP_HANDLE_H

#include "client_connection.h"
#include "http_types.h"

int is_valid_request(char *buff, char **raw_http_headers, char **raw_http_body);
int parse_request_line (char *line, http_request_line_t * request_line);
int parse_http_header ();
int parse_http_body ();
int parse_http_request (client_connection_t * client_connection);

#endif