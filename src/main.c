#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "8080"
#define BACK_LOGS 10
#define BUFFER_SIZE 8192

void
error_exit (char *msg)
{
  perror (msg);
  exit (EXIT_FAILURE);
}

void
print_addrinfo (struct addrinfo *host_info)
{
  struct addrinfo *temp_info;
  char host_ip[INET6_ADDRSTRLEN];

  for (temp_info = host_info; temp_info != NULL;
       temp_info = temp_info->ai_next)
    {
      void *addr = NULL;
      struct sockaddr_in *ipv4 = NULL;
      struct sockaddr_in6 *ipv6 = NULL;

      if (temp_info->ai_family == AF_INET)
	{
	  ipv4 = (struct sockaddr_in *) temp_info->ai_addr;
	  addr = &(ipv4->sin_addr);
	}
      else if (temp_info->ai_family == AF_INET6)
	{
	  ipv6 = (struct sockaddr_in6 *) temp_info->ai_addr;
	  addr = &(ipv6->sin6_addr);
	}
      else
	{
	  break;
	}

      inet_ntop (temp_info->ai_family, addr, host_ip, sizeof host_ip);
      printf ("%s \n", host_ip);

    }
}

void
handle_child (int s)
{
  (void) s;

  int saved_errno = errno;
  while (waitpid (-1, NULL, WNOHANG) > 0);

  errno = saved_errno;
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
handle_recv (int s, char buff[BUFFER_SIZE])
{
  int byte_recv_len = 0;
  int bytes_recv = 0;
  while ((bytes_recv =
	  recv (s, buff + byte_recv_len, BUFFER_SIZE - byte_recv_len - 1,
		0)) > 0)
    {
      byte_recv_len += bytes_recv;
      break;
    }
  if (bytes_recv == -1)
    {
      close (s);
      error_exit ("Error: Failed to recv from client");
    }
  buff[byte_recv_len] = '\0';
}

void
handle_send (int s, char buff[BUFFER_SIZE])
{
  int bytes_sent_total = 0;
  int bytes_left = strlen (buff);
  int bytes_sent = 0;

  while ((bytes_sent = send (s, buff + bytes_sent_total, bytes_left, 0)) > 0)
    {
      bytes_left -= bytes_sent;
      bytes_sent_total += bytes_sent;
    }
  if (bytes_sent == -1)
    {
      close (s);
      error_exit ("Error: Failed to recv from client");
    }
}

void
echo (int s)
{
  char buff[BUFFER_SIZE];
  handle_recv (s, buff);
  printf ("Server: Received data \n%s\n", buff);
  handle_send (s, buff);
  printf ("Server: sent data \n%s\n", buff);
}

int
main (int argc, char *argv[])
{
  int s, ns;
  struct addrinfo hint;
  struct addrinfo *host_info, *temp_info;
  struct sockaddr_storage client_info;
  socklen_t sin_size;
  int sock_opt = 1;
  char ip[INET6_ADDRSTRLEN];
  struct sigaction sa;


  memset (&hint, 0, sizeof hint);
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_flags = AI_PASSIVE;

  if (getaddrinfo (NULL, PORT, &hint, &host_info) != 0)
    {
      error_exit ("Error: Failed to get host addr info");
    }

  s =
    socket (host_info->ai_family, host_info->ai_socktype,
	    host_info->ai_protocol);
  if (s == -1)
    {
      error_exit ("Error: Failed to create main socket");
    }

  if (setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof (int)) == -1)
    {
      close (s);
      error_exit ("Error: Failed to set sock option");
    }

  if (bind (s, host_info->ai_addr, host_info->ai_addrlen) == -1)
    {
      close (s);
      error_exit ("Error: Failed to bind host");

    }

  if (listen (s, BACK_LOGS) == -1)
    {
      close (s);
      error_exit ("Error: Failed to listen");
    }

  sa.sa_handler = handle_child;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction (SIGCHLD, &sa, NULL) == -1)
    {
      close (s);
      error_exit ("Error: Failed config sigaction");
    }

  printf ("Server: 🚀 Started to run at port %s 🚀\n", PORT);
  printf ("Server: ⏳ Waiting for connection! ⏳\n");

  while (1)
    {
      sin_size = sizeof client_info;
      ns = accept (s, (struct sockaddr *) &client_info, &sin_size);
      if (ns == -1)
	{
	  perror ("Failed to accept client");
	  continue;
	}
      inet_ntop (client_info.ss_family,
		 get_in_addr ((struct sockaddr *) &client_info), ip,
		 sizeof ip);
      printf ("Server: got connection from %s\n", ip);

      if (!fork ())
	{
	  close (s);
	  echo (ns);
	  close (ns);
	  exit (EXIT_SUCCESS);
	}
      close (ns);
    }
  close (s);
  freeaddrinfo (host_info);
  return 0;
}
