#include "lib/moos.h"

// prints lines, words and bytes of fd
static void
count (int fd, const char *name)
{
  char buf[512];
  uint32_t lines = 0, words = 0, bytes = 0;
  int in_word = 0;
  int n;
  while ((n = read (fd, buf, sizeof (buf))) > 0)
    {
      for (int j = 0; j < n; j++)
        {
          char c = buf[j];
          lines += c == '\n';
          int space = c == ' ' || c == '\n' || c == '\t' || c == '\r';
          words += !space && !in_word;
          in_word = !space;
        }
      bytes += n;
    }
  print_int (lines);
  putchar (' ');
  print_int (words);
  putchar (' ');
  print_int (bytes);
  if (name)
    {
      putchar (' ');
      print (name);
    }
  putchar ('\n');
}

// counts lines, words and bytes of files, or of stdin without arguments
int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      count (STDIN, 0);
      return 0;
    }
  for (int i = 1; i < argc; i++)
    {
      int fd = open (argv[i], O_READ);
      if (fd < 0)
        {
          print_error (argv[i], fd);
          continue;
        }
      count (fd, argv[i]);
      close (fd);
    }
  return 0;
}
