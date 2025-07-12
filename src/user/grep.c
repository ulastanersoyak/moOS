#include "lib/moos.h"

#define LINE_MAX 512

static int
contains (const char *line, const char *word)
{
  for (; *line; line++)
    {
      const char *a = line;
      const char *b = word;
      while (*a && *b && *a == *b)
        {
          a++;
          b++;
        }
      if (!*b)
        {
          return 1;
        }
    }
  return 0;
}

// prints the lines of fd that contain word. returns the number of matches
static int
grep (int fd, const char *word)
{
  char line[LINE_MAX];
  uint32_t len = 0;
  int matches = 0;
  char buf[256];
  int n;
  while ((n = read (fd, buf, sizeof (buf))) > 0)
    {
      for (int i = 0; i < n; i++)
        {
          if (buf[i] != '\n' && len < LINE_MAX - 1)
            {
              line[len++] = buf[i];
              continue;
            }
          if (buf[i] != '\n')
            {
              continue; // overlong line, the rest is cut off
            }
          line[len] = '\0';
          if (contains (line, word))
            {
              print (line);
              putchar ('\n');
              matches++;
            }
          len = 0;
        }
    }
  line[len] = '\0';
  if (len > 0 && contains (line, word))
    {
      print (line);
      putchar ('\n');
      matches++;
    }
  return matches;
}

// prints lines containing a word, from files or stdin. exits with 1 if no
// line matched, like grep
int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      eprint ("usage: grep <word> [file]...\n");
      return 2;
    }
  int matches = 0;
  if (argc == 2)
    {
      matches = grep (STDIN, argv[1]);
    }
  for (int i = 2; i < argc; i++)
    {
      int fd = open (argv[i], O_READ);
      if (fd < 0)
        {
          print_error (argv[i], fd);
          continue;
        }
      matches += grep (fd, argv[1]);
      close (fd);
    }
  return matches ? 0 : 1;
}
