#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <errno.h>
#include <search.h>
#include <signal.h>
#include <pthread.h>

#include "global.h"
#include "utils.h"
#include "connection_type.h"

void handle_sigint (int sig);
void setup_server ();
void init_server(server_connection_t *server_connection);
void *start_server (void *arg);
void start_worker_server(server_connection_t * server_connection);
void close_server(server_connection_t *server_connection);

#endif