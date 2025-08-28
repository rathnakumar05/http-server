#ifndef _CLIENT_CONNECTION_H
#define _CLIENT_CONNECTION_H

#include <arpa/inet.h>

typedef struct
{
    int s;
    struct sockaddr_storage client_info;
    socklen_t sin_size;
} client_connection_t;

#endif