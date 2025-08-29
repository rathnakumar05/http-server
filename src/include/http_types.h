#ifndef _HTTP_TYPE_H
#define _HTTP_TYPE_H

typedef struct
{
    char method[16];
    char path[1024];
    char version[16];
} http_request_line_t;

#endif