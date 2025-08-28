#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

void
custom_error_exit (char *message)
{
  perror (message);
  exit (EXIT_FAILURE);
}
