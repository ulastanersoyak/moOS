#include "lib/moos.h"

#define MAX_ENTRIES 64

static void
print_two_digits (uint32_t num)
{
  putchar ('0' + num / 10 % 10);
  putchar ('0' + num % 10);
}

// fat dates: year - 1980 in bits 15-9, month 8-5, day 4-0
static void
print_date (uint16_t date, uint16_t time)
{
  print_int (1980 + (date >> 9));
  putchar ('-');
  print_two_digits ((date >> 5) & 0xF);
  putchar ('-');
  print_two_digits (date & 0x1F);
  putchar (' ');
  print_two_digits (time >> 11);
  putchar (':');
  print_two_digits ((time >> 5) & 0x3F);
}

static int
list (const char *path)
{
  static struct dirent entries[MAX_ENTRIES];
  int total = listdir (path, entries, MAX_ENTRIES);
  if (total < 0)
    {
      print_error (path[0] ? path : ".", total);
      return 1;
    }
  // the name column fits the longest (long file) name
  uint32_t width = 14;
  for (int i = 0; i < total; i++)
    {
      uint32_t len = strlen (entries[i].name) + entries[i].is_dir + 2;
      width = len > width ? len : width;
    }
  for (int i = 0; i < total; i++)
    {
      struct dirent *entry = &entries[i];
      char name[NAME_MAX + 2];
      uint32_t len = strlen (entry->name);
      memcpy (name, entry->name, len);
      if (entry->is_dir)
        {
          name[len++] = '/';
        }
      name[len] = '\0';
      print_padded (name, width);
      if (entry->is_dir)
        {
          print_padded ("<dir>", 14);
        }
      else
        {
          print_int (entry->size);
          print (" bytes");
          uint32_t digits = 1;
          for (uint32_t n = entry->size; n >= 10; n /= 10)
            {
              digits++;
            }
          for (uint32_t pad = digits + 6; pad < 14; pad++)
            {
              putchar (' ');
            }
        }
      print_date (entry->date, entry->time);
      putchar ('\n');
    }
  return 0;
}

// lists the working directory or the directories given as arguments
int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      return list ("");
    }
  int status = 0;
  for (int i = 1; i < argc; i++)
    {
      if (argc > 2)
        {
          print (argv[i]);
          print (":\n");
        }
      status |= list (argv[i]);
    }
  return status;
}
