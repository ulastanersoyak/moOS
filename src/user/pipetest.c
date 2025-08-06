#include "lib/moos.h"

// starts "sort" with its stdin connected to a pipe, writes lines into the
// pipe and waits for it. shows pipe, spawn and wait without the shell
int
main (void)
{
  int fds[2];
  if (pipe (fds) < 0)
    {
      eprint ("pipetest: pipe failed\n");
      return 1;
    }
  char *argv[] = { "sort", 0 };
  int io[2] = { fds[0], STDOUT };
  int pid = spawn ("sort", argv, io);
  if (pid < 0)
    {
      print_error ("sort", pid);
      return 1;
    }
  // the child has its own reference to the read end now
  close (fds[0]);
  const char *words[] = { "pear\n", "apple\n", "fig\n", "banana\n" };
  for (int i = 0; i < 4; i++)
    {
      write (fds[1], words[i], strlen (words[i]));
    }
  // closing the write end is the end of the input for sort
  close (fds[1]);
  int code = wait (pid);
  print ("pipetest: sort exited with ");
  print_int (code);
  putchar ('\n');
  return code;
}
