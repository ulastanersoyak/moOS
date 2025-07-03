#include "lib/moos.h"

static void
copy (int fd)
{
  char buf[512];
  int n;
  while ((n = read (fd, buf, sizeof (buf))) > 0)
    {
      write (STDOUT, buf, n);
    }
}

// prints files to stdout, or stdin without arguments
int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      copy (STDIN);
      return 0;
    }
  int status = 0;
  for (int i = 1; i < argc; i++)
    {
      int fd = open (argv[i], O_READ);
      if (fd < 0)
        {
          print_error (argv[i], fd);
          status = 1;
          continue;
        }
      copy (fd);
      close (fd);
    }
  return status;
}
