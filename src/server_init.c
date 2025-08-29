#include <stdio.h>
#include <sys/stat.h>

#include "config.h"
#include "error_handle.h"

void
init_server ()
{
  struct stat st = { 0 };

  if (stat (FOLDER_NAME, &st) == -1)
    {
      if (mkdir (FOLDER_NAME, 0755) == 0)
	{
	  printf ("Server: Folder created: %s\n", FOLDER_NAME);
	}
      else
	{
	  custom_error_exit ("Error: Failed create folder");
	}
    }
}
