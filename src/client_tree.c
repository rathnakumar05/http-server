#include "client_tree.h"

int
compare_clients (const void *a, const void *b)
{
  const client_connection_t *client_connection1 = a;
  const client_connection_t *client_connection2 = b;
  return client_connection1->s - client_connection2->s;
}

int
add_client (client_connection_t * client)
{
  void *client_connection = tsearch (client, &client_tree, compare_clients);
  if (client_connection == NULL)
    {
      perror ("Error: Failed to add client\n");
      return -1;
    }
  return 0;
}

int
del_client (client_connection_t * client)
{
  void *client_connection = tdelete (client, &client_tree, compare_clients);
  if (client_connection == NULL)
    {
      printf ("Error: client_connection not found\n");
      return -1;
    }

  return 0;
}

void
free_client (void *nodep)
{
  client_connection_t *client_connection = nodep;
  if (client_connection)
    {
      epoll_ctl (client_connection->epoll_fd, EPOLL_CTL_DEL,
		 client_connection->s, NULL);
      close (client_connection->s);
      free (client_connection);
    }
}

void
delete_client_all ()
{
  tdestroy (client_tree, free_client);
  client_tree = NULL;
}
