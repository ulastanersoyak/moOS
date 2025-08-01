#include "lib/moos.h"

// greets the names given as arguments, or asks for one
int
main (int argc, char **argv)
{
  char name[32];
  print ("hello from ring 3!\n");
  if (argc > 1)
    {
      for (int i = 1; i < argc; i++)
        {
          print ("nice to meet you, ");
          print (argv[i]);
          print ("\n");
        }
      return 0;
    }
  print ("what's your name? ");
  readline (name, sizeof (name));
  print ("nice to meet you, ");
  print (name[0] ? name : "stranger");
  print ("\n");
  print ("system uptime: ");
  print_int (uptime () / 1000);
  print (" s\n");
  return 0;
}
