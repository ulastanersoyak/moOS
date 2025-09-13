#include "lib/moos.h"

// prints the words given as arguments in a speech bubble above a cow
int
main (int argc, char **argv)
{
  char text[TERM_WIDTH];
  text[0] = '\0';
  for (int i = 1; i < argc; i++)
    {
      if (strlen (text) + strlen (argv[i]) + 2 > sizeof (text) - 4)
        {
          break;
        }
      if (i > 1)
        {
          strcat (text, " ");
        }
      strcat (text, argv[i]);
    }
  if (!text[0])
    {
      strcpy (text, "moo");
    }

  uint32_t len = strlen (text);
  print ("  ");
  for (uint32_t i = 0; i < len; i++)
    {
      putchar ('=');
    }
  print ("\n< ");
  setcolor (YELLOW);
  print (text);
  setcolor (WHITE);
  print (" >\n  ");
  for (uint32_t i = 0; i < len; i++)
    {
      putchar ('=');
    }
  print ("\n    ^__^\n");
  print ("    (oo)_______\n");
  print ("    (__)       )-/\n");
  print ("       ||----w |\n");
  print ("       ||     ||\n");
  return 0;
}
