#include "lib/moos.h"

// runs a program and prints how long it took
int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      eprint ("usage: time <program> [args]...\n");
      return 1;
    }
  uint32_t start = uptime ();
  int pid = spawn (argv[1], argv + 1, 0);
  if (pid < 0)
    {
      print_error (argv[1], pid);
      return 1;
    }
  int code = wait (pid);
  uint32_t elapsed = uptime () - start;
  eprint ("time: ");
  print_int (elapsed);
  eprint (" ms\n");
  return code;
}
