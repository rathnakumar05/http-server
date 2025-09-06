#include "http_response.h"

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
send_all (int s, char *buff, size_t buff_len)
{
  size_t bytes_sent_total = 0;
  size_t bytes_left = buff_len;
  size_t bytes_sent = 0;

  while (bytes_left > 0)
    {
      bytes_sent = send (s, buff + bytes_sent_total, bytes_left, 0);
      if (bytes_sent == -1)
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
      else
	{
	  bytes_left -= bytes_sent;
	  bytes_sent_total += bytes_sent;
	}
    }

  return 0;
}

int
send_file (int s, int f, size_t file_size)
{
  off_t offset = 0;
  size_t bytes_left = file_size;
  size_t bytes_sent = 0;

  while (bytes_left > 0)
    {
      bytes_sent = sendfile (s, f, &offset, file_size);
      if (bytes_sent == -1)
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
      else
	{
	  bytes_left -= bytes_sent;
	}
    }
  return 0;
}

FILE *
get_file (char *path, size_t *file_size, int *response_code)
{
  FILE *f;
  if (access (path, F_OK) != 0)
    {
      *response_code = 404;
      printf ("Error: file not found [%s]\n", path);
      return NULL;
    }

  f = fopen (path, "rb");

  if (f == NULL)
    {
      *response_code = 500;
      perror ("Server: Failed to open file");
      return NULL;
    }

  if (fseek (f, 0, SEEK_END) != 0)
    {
      *response_code = 500;
      perror ("Server: Failed to seek file");
      return NULL;
    }

  *file_size = ftell (f);
  if (*file_size == -1)
    {
      *response_code = 500;
      perror ("Server: Failed to ftell file");
      return NULL;
    }

  rewind (f);

  if (ferror (f))
    {
      *response_code = 500;
      perror ("Server: Failed to rewind file");
      return NULL;
    }

  *response_code = 200;
  return f;
}

void
handle_response (client_connection_t * client_connection)
{
  char buffer[BUFFER_SIZE];
  if (client_connection->response_code != 200)
    {
      snprintf (buffer, sizeof (buffer),
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: %s\r\n"
		"\r\n",
		client_connection->response_code,
		http_reason_phrase (client_connection->response_code),
		get_mime_type (".html"));
      send_all (client_connection->s, buffer, strlen (buffer));
      return;
    }

  char path[BUFFER_1K];

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
  size_t file_size;
  FILE *f = get_file (path, &file_size, &client_connection->response_code);

  if (f == NULL)
    {
      snprintf (buffer, sizeof (buffer),
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: %s\r\n"
		"\r\n",
		client_connection->response_code,
		http_reason_phrase (client_connection->response_code),
		get_mime_type (".html"));
      send_all (client_connection->s, buffer, strlen (buffer));
      return;
    }
  else
    {
      snprintf (buffer, sizeof (buffer),
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %zu\r\n"
		"\r\n",
		client_connection->response_code,
		http_reason_phrase (client_connection->response_code),
		get_mime_type (path), file_size);
      send_all (client_connection->s, buffer, strlen (buffer));
    }
  send_file (client_connection->s, fileno (f), file_size);
  fclose (f);
  return;
}
