#ifndef _CONNECTION_H
#define _CONNECTION_H

#include <netdb.h>
#include <sys/epoll.h>

#include "config.h"

typedef struct
{
    int s;
    struct addrinfo hint;
    struct addrinfo *host_info;
    int epoll_fd;
    struct epoll_event *events;
} server_connection_t;

typedef enum SOCKET_STATUS
{
    READ,
    SEND,
    CLOSE,
} socket_status_t;

typedef struct
{
    char method[16];
    char path[BUFFER_1K];
    char version[16];
} http_request_line_t;

typedef struct
{
    int s;
    struct sockaddr_storage client_info;
    socklen_t sin_size;
    char buffer[BUFFER_2K];
    http_request_line_t request_line;
    socket_status_t socket_status;
    int response_code;
    int epoll_fd;

} client_connection_t;

#endif