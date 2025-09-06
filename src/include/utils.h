#ifndef _UTILS_H
#define _UTILS_H

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <arpa/inet.h>

int set_non_blocking(int fd);
void perror_exit(char *message);
void * get_in_addr (struct sockaddr *sa);

#endif