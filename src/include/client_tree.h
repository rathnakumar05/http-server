#ifndef _CLIENT_TREE
#define _CLIENT_TREE
#define _GNU_SOURCE
#include <stdio.h>
#include <search.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"
#include "connection_type.h"

int compare_clients(const void *a, const void *b);
int add_client(client_connection_t *client);
int del_client(client_connection_t *client);
void free_client(void *nodep);
void delete_client_all();

#endif