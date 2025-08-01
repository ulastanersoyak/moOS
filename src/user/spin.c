#include "lib/moos.h"

// busy loop without syscalls. ctrl+c reaches it through the keyboard irq
int
main (void)
{
  print ("spinning forever, press ctrl+c to stop\n");
  for (;;)
    {
    }
  return 0;
}
