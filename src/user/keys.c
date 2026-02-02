#include "lib/moos.h"

// prints every key going down and up until esc is pressed, shows what
// games get from key_event

static const char *special[] = {
  "up",    "down", "left",  "right", "home",    "end",       "delete",
  "ctrl",  "shift", "alt",  "f1",    "f2",      "f3",        "f4",
  "f5",    "f6",   "f7",    "f8",    "f9",      "f10",       "f11",
  "f12",   "page up", "page down", "insert",
};

static void
print_key (int key)
{
  switch (key)
    {
    case '\n':
      print ("enter");
      return;
    case 27:
      print ("esc");
      return;
    case '\b':
      print ("backspace");
      return;
    case '\t':
      print ("tab");
      return;
    case ' ':
      print ("space");
      return;
    }
  if (key >= KEY_UP && key <= KEY_INSERT)
    {
      print (special[key - KEY_UP]);
    }
  else if (isprint (key))
    {
      putchar (key);
    }
  else
    {
      printf ("%x", key);
    }
}

int
main (void)
{
  key_event (1);
  print ("keys: press keys, esc quits\n");
  while (1)
    {
      int event = key_event (0);
      if (event < 0)
        {
          sleep (10);
          continue;
        }
      int key = event & ~KEY_RELEASED;
      print (event & KEY_RELEASED ? "up " : "down ");
      print_key (key);
      putchar ('\n');
      if (key == 27 && !(event & KEY_RELEASED))
        {
          return 0;
        }
    }
}
