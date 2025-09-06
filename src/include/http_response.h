#ifndef _HTTP_RESPONSE_H
#define _HTTP_RESPONSE_H

#include "http_server.h"

const char *http_reason_phrase(int code);
const char *get_mime_type(const char *path);
int send_all(int s, char *buff, size_t buff_len);
int send_file(int s, int f, size_t file_size);
FILE *get_file(char *path, size_t *file_size, int *response_code);
void handle_response(client_connection_t *client_connection);

#endif