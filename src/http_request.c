#include "http_request.h"

int
handle_recv (int s, char *buffer, int buffer_size, int *response_code)
{
  int byte_recv_len = 0;
  int bytes_recv = 0;
  while (1)
    {
      bytes_recv =
	recv (s, buffer + byte_recv_len, buffer_size - byte_recv_len - 1, 0);
      if (bytes_recv == -1)
	{
	  if (errno == EAGAIN || errno == EWOULDBLOCK)
	    {
	      break;
	    }
	  else
	    {
	      perror ("Error: Failed to read client data");
	      *response_code = 500;
	      return -1;
	    }
	}
      else if (bytes_recv == 0)
	{
	  break;
	}
      byte_recv_len += bytes_recv;
      if (byte_recv_len >= buffer_size - 1)
	{
	  printf ("Error: Data size exceed the limit [%d]\n", buffer_size);
	  *response_code = 400;
	  return -1;
	}
    }
  *response_code = 200;
  return byte_recv_len;
}

int
parse_request_line (char *buffer, http_request_line_t * request_line)
{
  char *head_end = strstr (buffer, "\r\n\r\n");

  if (!head_end)
    {
      printf ("Error: Invalid header: %s\n", buffer);
      return -1;
    }

  char *line = strtok (buffer, "\r\n");

  if (line == NULL)
    {
      printf ("Error: Failed to token string\n");
      return -1;
    }
  if (sscanf
      (line, "%15s %1023s %15s", request_line->method, request_line->path,
       request_line->version) != 3)
    {
      printf ("Error: Failed to parse request line %s\n", buffer);
      return -1;
    }

  return 0;
}

void
handle_request (client_connection_t * client_connection)
{
  int byte_recv_len =
    handle_recv (client_connection->s, client_connection->buffer,
		 sizeof (client_connection->buffer),
		 &client_connection->response_code);
  if (byte_recv_len == -1)
    {
      return;
    }
  client_connection->buffer[byte_recv_len] = '\0';
  printf ("Server: Received data from the client %s\n",
	  client_connection->buffer);

  if (parse_request_line
      (client_connection->buffer, &client_connection->request_line) == -1)

    {
      client_connection->response_code = 400;
      return;
    }

  if (strcmp (client_connection->request_line.method, "GET") != 0)
    {
      client_connection->response_code = 400;
    }

  return;
}
