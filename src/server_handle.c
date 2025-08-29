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
#include "http_handle.h"


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
handle_recv (client_connection_t * client_connection)
{
  int byte_recv_len = 0;
  int bytes_recv = 0;
  while ((bytes_recv =
	  recv (client_connection->s, client_connection->buff + byte_recv_len,
		BUFFER_SIZE - byte_recv_len - 1, 0)) > 0)
    {
      byte_recv_len += bytes_recv;
      break;
    }
  if (bytes_recv == -1)
    {
      close (client_connection->s);
      custom_error_exit ("Error: Failed to recv from client");
    }
  client_connection->buff[byte_recv_len] = '\0';
}

int
send_all (int s, char *buff, size_t buff_len)
{
  size_t bytes_sent_total = 0;
  size_t bytes_left = buff_len;
  size_t bytes_sent = 0;

  while ((bytes_sent = send (s, buff + bytes_sent_total, bytes_left, 0)) > 0)
    {
      bytes_left -= bytes_sent;
      bytes_sent_total += bytes_sent;
    }
  if (bytes_sent == -1)
    return -1;

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


void
http_response (client_connection_t * client_connection, char *path)
{
  int status_code = 200;
  FILE *f;
  size_t file_size;
  if (access (path, F_OK) == 0)
    {
      printf ("Hello\n");
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
      close (client_connection->s);
      custom_error_exit ("Server: Failed to send file");
    }

  if (status_code == 200)
    {
      char chunk[BUFFER_1K];
      size_t file_bytes_read;

      while ((file_bytes_read = fread (chunk, 1, sizeof chunk, f)) > 0)
	{
	  if (send_all (client_connection->s, chunk, file_bytes_read) == -1)
	    {
	      close (client_connection->s);
	      custom_error_exit ("Server: Failed to send file");
	    }
	}

      if (ferror (f))
	{
	  close (client_connection->s);
	  custom_error_exit ("Server: Failed to read file");
	}
    }

}


void
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
  http_response (client_connection, path);
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
	  handle_recv (&client_connection);
	  printf ("Server: Received data %s\n", client_connection.buff);
	  if (parse_http_request (&client_connection) == -1)
	    {
	      close (client_connection.s);
	      custom_error_exit ("Failed to parse HTTP request \n");
	    }
	  printf ("Server: method [%s] path [%s]\n",
		  client_connection.request_line.method,
		  client_connection.request_line.path);
	  handle_send (&client_connection);
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
