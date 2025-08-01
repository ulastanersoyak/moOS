#include "lib/moos.h"

// counts with the timer syscall. ctrl+c kills it
int
main (void)
{
  print ("counting to 10, press ctrl+c to stop\n");
  for (int i = 1; i <= 10; i++)
    {
      print_int (i);
      putchar (i < 10 ? ' ' : '\n');
      sleep (500);
    }
  return 10;
}
