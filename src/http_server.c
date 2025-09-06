#include "http_server.h"
#include "http_request.h"
#include "http_response.h"
#include "client_tree.h"

void
handle_sigint (int sig)
{
  exit_server = true;
}

void
setup_server ()
{
  struct sigaction sa;

  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = handle_sigint;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction (SIGINT, &sa, NULL) == -1)
    {
      perror_exit ("Server: Failed to sigaction(SIGINT)");
    }

  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = SIG_IGN;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction (SIGPIPE, &sa, NULL) == -1)
    {
      perror_exit ("Server: Failed to sigaction(SIGPIPE)");
    }
}

void
init_server (server_connection_t * server_connection)
{
  server_connection->epoll_fd = epoll_create1 (0);
  if (server_connection->epoll_fd == -1)
    {
      perror_exit ("Error: Failed to epoll instance");
    }
  int sock_opt = 1;

  memset (&server_connection->hint, 0, sizeof server_connection->hint);
  server_connection->hint.ai_family = AF_UNSPEC;
  server_connection->hint.ai_socktype = SOCK_STREAM;
  server_connection->hint.ai_flags = AI_PASSIVE;

  if (getaddrinfo (NULL, port, &server_connection->hint,
		   &server_connection->host_info) != 0)
    {
      perror_exit ("Error: Failed to get host addr info");
    }

  server_connection->s =
    socket (server_connection->host_info->ai_family,
	    server_connection->host_info->ai_socktype,
	    server_connection->host_info->ai_protocol);

  if (server_connection->s == -1)
    {
      perror_exit ("Error: Failed to create host socket connection");
    }

  if (setsockopt (server_connection->s, SOL_SOCKET, SO_REUSEADDR, &sock_opt,
		  sizeof (int)) == -1)
    {
      close (server_connection->s);
      perror_exit ("Error: Failed to set host socket option");
    }

  if (bind (server_connection->s, server_connection->host_info->ai_addr,
	    server_connection->host_info->ai_addrlen) == -1)
    {
      close (server_connection->s);
      perror_exit ("Error: Failed to bind host");
    }

  if (listen (server_connection->s, BACK_LOGS) == -1)
    {
      close (server_connection->s);
      perror_exit ("Error: Failed to listen");
    }

  if (set_non_blocking (server_connection->s) == -1)
    {
      close (server_connection->s);
      perror_exit ("Failed to set non-blocking (host)");
    }

  struct epoll_event epoll_event;
  epoll_event.events = EPOLLIN | EPOLLET;
  epoll_event.data.fd = server_connection->s;

  if (epoll_ctl
      (server_connection->epoll_fd, EPOLL_CTL_ADD, server_connection->s,
       &epoll_event) == -1)
    {
      close (server_connection->s);
      perror_exit ("Error: Failed to add host socket in epoll");
    }
}

void *
start_server (void *arg)
{
  server_connection_t *server_connection = (server_connection_t *) arg;

  printf ("Server: 🚀 Started to run at port %s 🚀\n", port);
  printf ("Server: ⏳ Waiting for connection! ⏳\n");

  struct epoll_event *events =
    (struct epoll_event *) malloc (sizeof (struct epoll_event) * MAX_EVENTS);

  if (events == NULL)
    {
      close (server_connection->s);
      perror_exit ("Server: Failed to malloc events");
    }
  char ip[INET6_ADDRSTRLEN];
  int fds;
  while (exit_server == false)
    {
      fds = epoll_wait (server_connection->epoll_fd, events, MAX_EVENTS, -1);
      if (fds == -1)
	{
	  if (errno != EINTR)
	    {
	      perror ("Server: Failed to epoll_wait");
	    }
	  continue;
	}

      for (int i = 0; i < fds; i++)
	{
	  if (events[i].data.fd == server_connection->s)
	    {
	      while (1)
		{
		  int s;
		  struct sockaddr_storage client_info;
		  socklen_t sin_size = sizeof client_info;
		  s =
		    accept (server_connection->s,
			    (struct sockaddr *) &client_info, &sin_size);
		  if (s == -1)
		    {
		      if (errno == EAGAIN | errno == EWOULDBLOCK)
			{
			  break;
			}
		      else
			{
			  perror ("Server: Failed to accept clients");
			  break;
			}
		    }

		  inet_ntop (client_info.ss_family,
			     get_in_addr ((struct sockaddr *) &client_info),
			     ip, sizeof ip);
		  printf ("Server: got connection from %s\n", ip);

		  if (set_non_blocking (s) == -1)
		    {
		      perror
			("Server: Failed to set non-blocking for client");
		      close (s);
		      continue;
		    }

		  struct epoll_event epoll_event;
		  epoll_event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
		  client_connection_t *client_connection =
		    (client_connection_t *)
		    malloc (sizeof (client_connection_t));

		  if (client_connection == NULL)
		    {
		      perror ("Server: Failed to malloc client_connection");
		      close (s);
		      continue;
		    }

		  client_connection->s = s;
		  client_connection->client_info = client_info;
		  client_connection->sin_size = sin_size;
		  client_connection->epoll_fd = server_connection->epoll_fd;
		  client_connection->socket_status = READ;
		  epoll_event.data.ptr = client_connection;
		  if (epoll_ctl (server_connection->epoll_fd, EPOLL_CTL_ADD,
				 client_connection->s, &epoll_event) == -1)
		    {
		      close (client_connection->s);
		      free (client_connection);
		      perror ("Error: Failed to add epoll_ctl (client)");
		      continue;
		    }
		  add_client (client_connection);
		}

	    }
	  else
	    {
	      client_connection_t *client_connection =
		(client_connection_t *) events[i].data.ptr;
	      if (client_connection->socket_status == READ)
		{
		  handle_request (client_connection);
		  client_connection->socket_status = SEND;
		  struct epoll_event epoll_event;
		  epoll_event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
		  epoll_event.data.ptr = client_connection;
		  if (epoll_ctl (server_connection->epoll_fd, EPOLL_CTL_MOD,
				 client_connection->s, &epoll_event) < 0)
		    {
		      del_client (client_connection);
		      epoll_ctl (server_connection->epoll_fd, EPOLL_CTL_DEL,
				 client_connection->s, NULL);
		      close (client_connection->s);
		      free (client_connection);
		      client_connection = NULL;
		      perror
			("Error: Failed to mod [read] epoll_ctl (client)");
		    }
		}
	      else if (client_connection->socket_status == SEND)
		{
		  handle_response (client_connection);
		  client_connection->socket_status = CLOSE;
		  struct epoll_event epoll_event;
		  epoll_event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
		  epoll_event.data.ptr = client_connection;
		  if (epoll_ctl (server_connection->epoll_fd, EPOLL_CTL_MOD,
				 client_connection->s, &epoll_event) < 0)
		    {
		      del_client (client_connection);
		      epoll_ctl (server_connection->epoll_fd, EPOLL_CTL_DEL,
				 client_connection->s, NULL);
		      close (client_connection->s);
		      free (client_connection);
		      client_connection = NULL;
		      perror
			("Error: Failed to mod [send] epoll_ctl (client)");
		      break;
		    }
		}
	      else
		{
		  del_client (client_connection);
		  epoll_ctl (server_connection->epoll_fd, EPOLL_CTL_DEL,
			     client_connection->s, NULL);
		  printf ("Server: closing client connection\n");
		  close (client_connection->s);
		  free (client_connection);
		  client_connection = NULL;
		}
	    }
	}
    }
  free (events);
  events = NULL;
  printf ("Hello from servers\n");
  return NULL;
}

void
start_worker_server (server_connection_t * server_connection)
{
  pthread_t threads[WORKERS_COUNT];
  for (int i = 0; i < WORKERS_COUNT; ++i)
    {
      if (pthread_create (&threads[i], NULL, start_server, server_connection)
	  == -1)
	{
	  perror ("Server: Failed to create thread");
	}
    }
  start_server ((void *) server_connection);
}

void
close_server (server_connection_t * server_connection)
{
  printf ("\nServer: 🛑 shutting down the server 🛑\n");
  delete_client_all ();
  close (server_connection->s);
  close (server_connection->epoll_fd);
}
