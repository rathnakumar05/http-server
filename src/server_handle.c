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
#include <errno.h>


#include "config.h"
#include "global.h"
#include "error_handle.h"
#include "server_connection.h"
#include "client_connection.h"
#include "http_handle.h"
#include <sys/epoll.h>
#include <fcntl.h>

//TODO: handle errors
void
setnonblocking (int fd)
{
  int old_option = fcntl (fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;

  fcntl (fd, F_SETFL, new_option);
}

//TODO: need to refactor
int epoll_fd;


void
server_connect (server_connection_t * server_connection)
{
  epoll_fd = epoll_create1 (0);

  if (epoll_fd == -1)
    {
      custom_error_exit ("Error: Failed to epoll instance");
    }
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

  setnonblocking (server_connection->s);
  struct epoll_event ep_event;
  ep_event.events = EPOLLIN | EPOLLET;
  ep_event.data.fd = server_connection->s;

  if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, server_connection->s, &ep_event) ==
      -1)
    {
      close (server_connection->s);
      custom_error_exit ("Error: Failed to add listen fd in epoll");
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

int
handle_recv (client_connection_t * client_connection)
{
  int byte_recv_len = 0;
  int bytes_recv = 0;
  while (1)
    {
      bytes_recv =
	recv (client_connection->s, client_connection->buff + byte_recv_len,
	      BUFFER_SIZE - byte_recv_len - 1, 0);
      if (bytes_recv == -1)
	{
	  if (errno == EAGAIN || errno == EWOULDBLOCK)
	    {
	      break;
	    }
	  else
	    {
	      perror ("Error: Failed to read client data");
	      return -1;
	    }
	}
      else if (bytes_recv == 0)
	{
	  break;
	}
      byte_recv_len += bytes_recv;
    }
  client_connection->buff[byte_recv_len] = '\0';
  printf ("%s", client_connection->buff);
  return 0;
}

int
send_all (int s, char *buff, size_t buff_len)
{
  size_t bytes_sent_total = 0;
  size_t bytes_left = buff_len;
  size_t bytes_sent = 0;

  while (1)
    {
      bytes_sent = send (s, buff + bytes_sent_total, bytes_left, 0);
      if (bytes_left == -1)
	{
	  if (errno == EAGAIN || errno == EWOULDBLOCK)
	    {
	      break;
	    }
	  else
	    {
	      return -1;
	    }
	}
      else if (bytes_left == 0)
	{
	  break;
	}
      else
	{
	  bytes_left -= bytes_sent;
	  bytes_sent_total += bytes_sent;
	}
    }

  return 0;
}

const char *
http_reason_phrase (int code)
{
  switch (code)
    {
    case 200:
      return "OK";
    case 201:
      return "Created";
    case 204:
      return "No Content";
    case 301:
      return "Moved Permanently";
    case 302:
      return "Found";
    case 304:
      return "Not Modified";
    case 400:
      return "Bad Request";
    case 403:
      return "Forbidden";
    case 404:
      return "Not Found";
    case 500:
      return "Internal Server Error";
    case 501:
      return "Not Implemented";
    case 503:
      return "Service Unavailable";
    default:
      return "Unknown";
    }
}

const char *
get_mime_type (const char *path)
{
  const char *ext = strrchr (path, '.');
  if (!ext)
    return "application/octet-stream";

  if (strcmp (ext, ".html") == 0 || strcmp (ext, ".htm") == 0)
    return "text/html";
  if (strcmp (ext, ".css") == 0)
    return "text/css";
  if (strcmp (ext, ".js") == 0)
    return "application/javascript";
  if (strcmp (ext, ".json") == 0)
    return "application/json";
  if (strcmp (ext, ".png") == 0)
    return "image/png";
  if (strcmp (ext, ".jpg") == 0 || strcmp (ext, ".jpeg") == 0)
    return "image/jpeg";
  if (strcmp (ext, ".gif") == 0)
    return "image/gif";
  if (strcmp (ext, ".svg") == 0)
    return "image/svg+xml";
  if (strcmp (ext, ".ico") == 0)
    return "image/x-icon";
  if (strcmp (ext, ".txt") == 0)
    return "text/plain";
  if (strcmp (ext, ".pdf") == 0)
    return "application/pdf";

  return "application/octet-stream";
}


int
http_response (client_connection_t * client_connection, char *path)
{
  int status_code = 200;
  FILE *f;
  size_t file_size;
  if (access (path, F_OK) == 0)
    {
      f = fopen (path, "rb");
      if (f == NULL)
	{
	  status_code = 500;
	  perror ("Server: Failed to open file");
	}

      if (fseek (f, 0, SEEK_END) != 0)
	{
	  status_code = 500;
	  perror ("Server: Failed to seek file");
	}

      file_size = ftell (f);
      if (file_size == -1)
	{
	  status_code = 500;
	  perror ("Server: Failed to ftell file");
	}

      rewind (f);

      if (ferror (f))
	{
	  status_code = 500;
	  perror ("Server: Failed to rewind file");
	}
    }
  else
    {

      status_code = 404;
      printf ("Server: File not found \n");
    }


  char buff[BUFFER_SIZE];

  if (status_code != 200)
    {
      snprintf (buff, sizeof (buff),
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: %s\r\n"
		"\r\n",
		status_code, http_reason_phrase (status_code),
		get_mime_type (path));
    }
  else
    {
      snprintf (buff, sizeof (buff),
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %zu\r\n"
		"\r\n",
		status_code, http_reason_phrase (status_code),
		get_mime_type (path), file_size);
    }

  if (send_all (client_connection->s, buff, strlen (buff)) == -1)
    {
      printf ("Server: Failed to send file\n");
      return -1;
    }
  else
    {
      if (status_code == 200)
	{
	  char chunk[BUFFER_1K];
	  size_t file_bytes_read;

	  while ((file_bytes_read = fread (chunk, 1, sizeof chunk, f)) > 0)
	    {
	      if (send_all (client_connection->s, chunk, file_bytes_read) ==
		  -1)
		{
		  fclose (f);
		  printf ("Server: Failed to send file\n");
		  return -1;
		}
	    }

	  if (ferror (f))
	    {
	      fclose (f);
	      printf ("Server: Failed to read file\n");
	      return -1;
	    }
	}
    }

  return 0;

}


int
handle_send (client_connection_t * client_connection)
{
  char path[BUFFER_2K];

  snprintf (path, sizeof (path), "%s", FOLDER_NAME);

  if (strcmp (client_connection->request_line.path, "/") == 0)
    {
      strncat (path, "/index.html", sizeof (path) - strlen (path) - 1);
    }
  else
    {
      strncat (path, client_connection->request_line.path,
	       sizeof (path) - strlen (path) - 1);
    }

  printf ("Server: path [%s]\n", path);
  return http_response (client_connection, path);
}

void
handle_request (client_request_t * client_request)
{
  if (client_request->status == RECV)
    {
      if (handle_recv (&client_request->client_connection) != -1)
	{
	  printf ("Server: Received data %s\n",
		  client_request->client_connection.buff);
	  if (parse_http_request (&client_request->client_connection) == -1)
	    {
	      printf ("Error: Failed to parse HTTP request \n");
	      client_request->status = CLOSE;
	    }
	  else
	    {
	      printf ("Server: Received data %s\n",
		      client_request->client_connection.buff);
	      printf ("Server: method [%s] path [%s]\n",
		      client_request->client_connection.request_line.method,
		      client_request->client_connection.request_line.path);
	      client_request->status = SEND;
	    }

	}
      else
	{
	  printf
	    ("Error: Due to handle_recv failure, client connection is closed\n");
	  client_request->status = CLOSE;
	}
    }
  else if (client_request->status == SEND)
    {
      handle_send (&client_request->client_connection);
      client_request->status = CLOSE;
    }

}

void
start_server (server_connection_t * server_connection)
{
  printf ("Server: 🚀 Started to run at port %s 🚀\n", PORT);
  printf ("Server: ⏳ Waiting for connection! ⏳\n");

  struct epoll_event *events =
    (struct epoll_event *) malloc (sizeof (struct epoll_event) * MAX_EVENTS);
  if (events == NULL)
    {
      close (server_connection->s);
      custom_error_exit ("Server: Failed to malloc events");
    }
  struct epoll_event ep_event;
  char ip[INET6_ADDRSTRLEN];
  client_connection_t client_connection;
  int fds;
  while (exit_server == false)
    {
      fds = epoll_wait (epoll_fd, events, MAX_EVENTS, -1);

      if (fds == -1)
	{
	  close (server_connection->s);
	  custom_error_exit ("Server: Failed to epoll_wait");
	}

      for (int i = 0; i < fds; i++)
	{
	  if (events[i].data.fd == server_connection->s)
	    {
	      while (1)
		{
		  client_connection.sin_size =
		    sizeof client_connection.client_info;
		  client_connection.s =
		    accept (server_connection->s,
			    (struct sockaddr *)
			    &client_connection.client_info,
			    &client_connection.sin_size);

		  if (client_connection.s < 0)
		    {
		      if (errno == EAGAIN | errno == EWOULDBLOCK)
			{
			  break;
			}
		      else
			{
			  perror ("Server: Failed to accept client");
			  break;
			}
		    }
		  inet_ntop (client_connection.client_info.ss_family,
			     get_in_addr ((struct sockaddr *)
					  &client_connection.client_info), ip,
			     sizeof ip);
		  printf ("Server: got connection from %s\n", ip);
		  setnonblocking (client_connection.s);

		  ep_event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
		  client_request_t *client_request =
		    (client_request_t *) malloc (sizeof (client_request_t));
		  if (client_request == NULL)
		    {
		      //TODO handle error
		      custom_error_exit
			("Error: failed to malloc client_request");
		    }
		  client_request->client_connection = client_connection;
		  client_request->status = RECV;
		  ep_event.data.ptr = client_request;
		  if (epoll_ctl
		      (epoll_fd, EPOLL_CTL_ADD, client_connection.s,
		       &ep_event) == -1)
		    {
		      perror ("epoll_ctl");
		      continue;
		    }

		}
	    }
	  else
	    {
	      client_request_t *client_request =
		(client_request_t *) events[i].data.ptr;
	      handle_request (client_request);
	      if (client_request->status == RECV)
		{
		  ep_event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
		  ep_event.data.ptr = client_request;
		  if (epoll_ctl
		      (epoll_fd, EPOLL_CTL_MOD,
		       client_request->client_connection.s, &ep_event) < 0)
		    {
		      perror ("Error: Failed to epoll_ctl");
		      continue;
		    }
		}
	      else if (client_request->status == SEND)
		{
		  ep_event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
		  ep_event.data.ptr = client_request;
		  if (epoll_ctl
		      (epoll_fd, EPOLL_CTL_MOD,
		       client_request->client_connection.s, &ep_event) < 0)
		    {
		      perror ("epoll_ctl");
		      continue;
		    }
		}
	      else if (client_request->status == CLOSE)
		{
		  printf ("Server: closing client connection\n");
		  close (client_request->client_connection.s);
		  free (client_request);
		}
	    }
	}
    }
}

void
close_server (server_connection_t * server_connection)
{
  close (server_connection->s);
  freeaddrinfo (server_connection->host_info);
}
