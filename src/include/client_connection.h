#ifndef _CLIENT_CONNECTION_H
#define _CLIENT_CONNECTION_H

#include <arpa/inet.h>

#include "config.h"
#include "http_types.h"
typedef struct
{
    int s;
    struct sockaddr_storage client_info;
    socklen_t sin_size;
    char buff[BUFFER_SIZE];
    char *raw_http_headers;
    char *raw_http_body;
    http_request_line_t request_line;
} client_connection_t;

typedef struct
{
    client_connection_t client_connection;
    int status;
} client_request_t;


#endif