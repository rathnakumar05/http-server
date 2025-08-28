#include "config.h"
#include "global.h"
#include "server_connection.h"
#include "server_handle.h"

int
main (int argc, char *argv[])
{
  server_connection_t server_connection;
  server_connect (&server_connection);
  start_server (&server_connection);
  close_server (&server_connection);
  return 0;
}
