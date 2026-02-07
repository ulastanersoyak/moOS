#include "lib/moos.h"

// checks the c library of user programs against known results

static int failures = 0;

static void
expect (const char *what, const char *got, const char *want)
{
  if (strcmp (got, want) != 0)
    {
      printf ("libctest: %s gave \"%s\", expected \"%s\"\n", what, got, want);
      failures++;
    }
}

static void
check_printf (void)
{
  char buf[64];
  snprintf (buf, sizeof (buf), "%i|%5d|%-5d|%05d|%+d", 42, -7, 3, -42, 9);
  expect ("ints", buf, "42|   -7|3    |-0042|+9");
  snprintf (buf, sizeof (buf), "%x|%X|%08x|%o|%p", 255, 255, 0xbeef, 8,
            (void *)0x1000);
  expect ("hex", buf, "ff|FF|0000beef|10|0x1000");
  snprintf (buf, sizeof (buf), "%ld|%lu|%lld|%llu", -5L, 6UL, -1234567890123LL,
            18446744073709551615ULL);
  expect ("longs", buf, "-5|6|-1234567890123|18446744073709551615");
  snprintf (buf, sizeof (buf), "%.3s|%10.2s|%*d|%-*s|", "abcdef", "xyz", 4, 1,
            3, "a");
  expect ("precision", buf, "abc|        xy|   1|a  |");
  snprintf (buf, sizeof (buf), "%f|%.2f|%.0f|%6.1f", 3.14159, -2.005, 2.5,
            0.25);
  expect ("floats", buf, "3.141590|-2.00|3|   0.3");
  snprintf (buf, sizeof (buf), "%.3d|%c%c|%%|%s", 7, 'o', 'k', (char *)0);
  expect ("misc", buf, "007|ok|%|(null)");
  int len = snprintf (buf, 4, "%s", "truncated");
  expect ("truncation", buf, "tru");
  if (len != 9)
    {
      printf ("libctest: snprintf returned %d, expected 9\n", len);
      failures++;
    }
}

int
main (void)
{
  check_printf ();
  printf ("libctest: %s\n", failures ? "failed" : "ok");
  return failures != 0;
}
