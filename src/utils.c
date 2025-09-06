#include "utils.h"

int
set_non_blocking (int fd)
{
  int flags;

  if ((flags = fcntl (fd, F_GETFL, 0)) == -1)
    {
      return -1;
    }

  if (flags & O_NONBLOCK)
    {
      return 0;
    }

  if (fcntl (fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
      return -1;
    }

  return 0;
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
perror_exit (char *message)
{
  perror (message);
  exit (EXIT_FAILURE);
}
