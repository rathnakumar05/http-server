#include <stdio.h>
#include <string.h>

#include "client_connection.h"
#include "http_handle.h"

int
is_valid_request (char *buff, char **raw_http_headers, char **raw_http_body)
{
  char *header_end = strstr (buff, "\r\n\r\n");
  if (header_end)
    {
      *header_end = '\0';
      *raw_http_headers = buff;
      *raw_http_body = header_end + 4;
      return 0;
    }
  printf ("Error: Invalid HTTP request \n");
  return -1;
}

int
parse_request_line (char *line, http_request_line_t * request_line)
{
  if (sscanf
      (line, "%15s %1023s %15s", request_line->method, request_line->path,
       request_line->version) == 3)
    {
      return 1;
    }
  printf ("Error: Failed to parse request line \n");
  return -1;
}

int
parse_http_header ()
{
  //TODO
  return 0;
}

int
parse_http_body ()
{
  //TODO
  return 0;
}

int
parse_http_request (client_connection_t * client_connection)
{
  if (is_valid_request
      (client_connection->buff, &client_connection->raw_http_headers,
       &client_connection->raw_http_body) == -1)
    return -1;

  char *line = strtok (client_connection->raw_http_headers, "\r\n");

  if (line == NULL)
    {
      printf ("Error: Failed to token string\n");
      return -1;
    }

  if (parse_request_line (line, &client_connection->request_line) == -1)
    return -1;

  return 0;
}
