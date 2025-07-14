#include "lib/moos.h"

// copies a file
int
main (int argc, char **argv)
{
  if (argc != 3)
    {
      print ("usage: cp <from> <to>\n");
      return 1;
    }
  int in = open (argv[1], O_READ);
  if (in < 0)
    {
      print_error (argv[1], in);
      return 1;
    }
  int out = open (argv[2], O_WRITE);
  if (out < 0)
    {
      print_error (argv[2], out);
      return 1;
    }
  char buf[1024];
  int n;
  while ((n = read (in, buf, sizeof (buf))) > 0)
    {
      if (write (out, buf, n) != n)
        {
          print ("cp: write failed\n");
          return 1;
        }
    }
  close (in);
  close (out);
  return 0;
}
