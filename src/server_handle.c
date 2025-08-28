#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>


#include "config.h"
#include "global.h"
#include "error_handle.h"
#include "server_connection.h"
#include "client_connection.h"


void
server_connect (server_connection_t * server_connection)
{
  printf ("%d", exit_server);
  int sock_opt = 1;

  memset (&server_connection->hint, 0, sizeof server_connection->hint);
  server_connection->hint.ai_family = AF_UNSPEC;
  server_connection->hint.ai_socktype = SOCK_STREAM;
  server_connection->hint.ai_flags = AI_PASSIVE;

  if (getaddrinfo
      (NULL, PORT, &server_connection->hint,
       &server_connection->host_info) != 0)
    {
      custom_error_exit ("Error: Failed to get host addr info");
    }

  server_connection->s =
    socket (server_connection->host_info->ai_family,
	    server_connection->host_info->ai_socktype,
	    server_connection->host_info->ai_protocol);

  if (server_connection->s == -1)
    {
      custom_error_exit ("Error: Failed to create main socket");
    }

  if (setsockopt
      (server_connection->s, SOL_SOCKET, SO_REUSEADDR, &sock_opt,
       sizeof (int)) == -1)
    {
      close (server_connection->s);
      custom_error_exit ("Error: Failed to set sock option");
    }

  if (bind
      (server_connection->s, server_connection->host_info->ai_addr,
       server_connection->host_info->ai_addrlen) == -1)
    {
      close (server_connection->s);
      custom_error_exit ("Error: Failed to bind host");
    }

  if (listen (server_connection->s, BACK_LOGS) == -1)
    {
      close (server_connection->s);
      custom_error_exit ("Error: Failed to listen");
    }
}

void *
get_in_addr (struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
    {
      return &(((struct sockaddr_in *) sa)->sin_addr);
    }

  return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void
start_server (server_connection_t * server_connection)
{
  printf ("Server: 🚀 Started to run at port %s 🚀\n", PORT);
  printf ("Server: ⏳ Waiting for connection! ⏳\n");

  char ip[INET6_ADDRSTRLEN];
  client_connection_t client_connection;
  while (exit_server == false)
    {
      client_connection.sin_size = sizeof client_connection.client_info;
      client_connection.s =
	accept (server_connection->s,
		(struct sockaddr *) &client_connection.client_info,
		&client_connection.sin_size);

      if (client_connection.s == -1)
	{
	  perror ("Failed to accept client");
	  continue;
	}

      inet_ntop (client_connection.client_info.ss_family,
		 get_in_addr ((struct sockaddr *)
			      &client_connection.client_info), ip, sizeof ip);
      printf ("Server: got connection from %s\n", ip);

      if (!fork ())
	{
	  close (server_connection->s);
	  close (client_connection.s);
	  exit (EXIT_SUCCESS);
	}
      close (client_connection.s);
    }
}

void
close_server (server_connection_t * server_connection)
{
  close (server_connection->s);
  freeaddrinfo (server_connection->host_info);
}
