#include "lib/moos.h"

// draws the moOS moose with the arguments (or "moOS") next to its head
int
main (int argc, char **argv)
{
  char text[37];
  text[0] = '\0';
  for (int i = 1; i < argc; i++)
    {
      if (strlen (text) + strlen (argv[i]) + 2 > sizeof (text))
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
      strcpy (text, "moOS");
    }

  print ("                                  ___            ___\n");
  print ("                                 /   \\          /   \\\n");
  print ("                                 \\_   \\        /  __/\n");
  print ("                                  _\\   \\      /  /__\n");
  print ("                                  \\___  \\____/   __/\n");
  print ("                                      \\_       _/\n");
  putchar (' ');
  setcolor (YELLOW);
  print_padded (text, 36);
  setcolor (WHITE);
  print (" | @ @  \\_\n");
  print ("                                        |\n");
  print ("                                      _/     /\\\n");
  print ("                                     /o)  (o/\\ \\_\n");
  print ("                                     \\_____/ /\n");
  print ("                                       \\____/\n");
  return 0;
}
