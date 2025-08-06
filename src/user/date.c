#include "lib/moos.h"

static void
print_two_digits (uint32_t num)
{
  putchar ('0' + num / 10 % 10);
  putchar ('0' + num % 10);
}

// prints the date and time of the real time clock
int
main (void)
{
  struct datetime now;
  time (&now);
  print_int (now.year);
  putchar ('-');
  print_two_digits (now.month);
  putchar ('-');
  print_two_digits (now.day);
  putchar (' ');
  print_two_digits (now.hour);
  putchar (':');
  print_two_digits (now.minute);
  putchar (':');
  print_two_digits (now.second);
  putchar ('\n');
  return 0;
}
