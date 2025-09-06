#define _GNU_SOURCE

#include <getopt.h>

#include "http_server.h"

int
main (int argc, char *argv[])
{
  int opt;

  while ((opt = getopt (argc, argv, "p:")) != -1)
    {
      switch (opt)
	{
	case 'p':
	  port = optarg;
	  break;
	}
    }

  server_connection_t server_connection;
  setup_server ();
  init_server (&server_connection);
  start_server (&server_connection);
  close_server (&server_connection);
  exit (EXIT_SUCCESS);
}
