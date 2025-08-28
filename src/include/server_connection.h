#ifndef _SERVER_CONNECTION_H
#define _SERVER_CONNECTION_H

#include <netdb.h>

typedef struct
{
    int s;
    struct addrinfo hint;
    struct addrinfo *host_info;
} server_connection_t;

#endif