#include "lib/moos.h"

// prints its arguments
int
main (int argc, char **argv)
{
  for (int i = 1; i < argc; i++)
    {
      if (i > 1)
        {
          putchar (' ');
        }
      print (argv[i]);
    }
  putchar ('\n');
  return 0;
}
