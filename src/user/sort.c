#include "lib/moos.h"

// reads everything from fd into a growing heap buffer
static char *
read_all (int fd, uint32_t *size)
{
  uint32_t capacity = 1024;
  uint32_t len = 0;
  char *text = malloc (capacity);
  int n;
  while (text && (n = read (fd, text + len, capacity - len - 1)) > 0)
    {
      len += n;
      if (capacity - len - 1 == 0)
        {
          capacity *= 2;
          text = realloc (text, capacity);
        }
    }
  if (text)
    {
      text[len] = '\0';
    }
  *size = len;
  return text;
}

// prints the lines of a file (or stdin) in sorted order. the text and the
// line table live on the heap
int
main (int argc, char **argv)
{
  int fd = STDIN;
  if (argc > 1)
    {
      fd = open (argv[1], O_READ);
      if (fd < 0)
        {
          print_error (argv[1], fd);
          return 1;
        }
    }
  uint32_t size;
  char *text = read_all (fd, &size);
  if (!text)
    {
      eprint ("sort: out of memory\n");
      return 1;
    }

  // split into lines, growing the table as needed
  uint32_t count = 0;
  uint32_t capacity = 16;
  char **lines = malloc (capacity * sizeof (char *));
  char *line = text;
  for (uint32_t i = 0; i <= size; i++)
    {
      if (text[i] != '\n' && text[i] != '\0')
        {
          continue;
        }
      text[i] = '\0';
      // empty lines are dropped
      if (line < text + i)
        {
          if (count == capacity)
            {
              capacity *= 2;
              lines = realloc (lines, capacity * sizeof (char *));
            }
          lines[count++] = line;
        }
      line = text + i + 1;
    }

  // insertion sort
  for (uint32_t i = 1; i < count; i++)
    {
      char *key = lines[i];
      int j = i - 1;
      while (j >= 0 && strcmp (lines[j], key) > 0)
        {
          lines[j + 1] = lines[j];
          j--;
        }
      lines[j + 1] = key;
    }
  for (uint32_t i = 0; i < count; i++)
    {
      print (lines[i]);
      putchar ('\n');
    }
  free (lines);
  free (text);
  return 0;
}
