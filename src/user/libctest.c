#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

// checks the c library of user programs (src/user/lib and src/user/libc)
// against known results

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
expect_int (const char *what, long got, long want)
{
  if (got != want)
    {
      printf ("libctest: %s gave %ld, expected %ld\n", what, got, want);
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
  expect_int ("snprintf length", len, 9);
  sprintf (buf, "%s-%d", "sprintf", 1);
  expect ("sprintf", buf, "sprintf-1");
}

static void
check_strings (void)
{
  char buf[32];
  expect ("strstr", strstr ("doom guy", "guy"), "guy");
  expect ("strrchr", strrchr ("a/b/c", '/'), "/c");
  expect_int ("strcasecmp", strcasecmp ("DooM", "doom"), 0);
  char *copy = strdup ("copied");
  expect ("strdup", copy, "copied");
  free (copy);
  strncpy (buf, "abc", sizeof (buf));
  strncat (buf, "defgh", 2);
  expect ("strncpy and strncat", buf, "abcde");
  expect_int ("strspn", strspn ("aabbc", "ab"), 4);
  expect_int ("isxdigit", isxdigit ('F') && !isxdigit ('g'), 1);
  strcpy (buf, "a,b,,c");
  char *first = strtok (buf, ",");
  char *second = strtok (0, ",");
  char *third = strtok (0, ",");
  expect ("strtok", third, "c");
  expect ("strtok first", first, "a");
  expect ("strtok second", second, "b");
}

static int
compare_ints (const void *a, const void *b)
{
  return *(const int *)a - *(const int *)b;
}

static void
check_numbers (void)
{
  char *end;
  expect_int ("strtol hex", strtol ("0x1A", 0, 0), 26);
  expect_int ("strtol octal", strtol ("077", 0, 0), 63);
  expect_int ("strtol", strtol ("  -42abc", &end, 10), -42);
  expect ("strtol end", end, "abc");
  expect_int ("strtoul", strtoul ("4294967295", 0, 10), 4294967295UL);
  expect_int ("strtod", (long)(strtod ("3.25e2", 0) * 10), 3250);
  expect_int ("atof", (long)(atof ("-0.5") * 100), -50);
  int values[] = { 5, -3, 9, 0, 7, 1 };
  qsort (values, 6, sizeof (int), compare_ints);
  char buf[32];
  snprintf (buf, sizeof (buf), "%d %d %d %d %d %d", values[0], values[1],
            values[2], values[3], values[4], values[5]);
  expect ("qsort", buf, "-3 0 1 5 7 9");
}

static void
check_scanf (void)
{
  int a = 0, b = 0, c = 0;
  char word[16] = "";
  expect_int ("sscanf count",
              sscanf ("12 0x1f hello 077", "%d %i %5s %o", &a, &b, word, &c),
              4);
  expect_int ("sscanf %d", a, 12);
  expect_int ("sscanf %i", b, 31);
  expect ("sscanf %s", word, "hello");
  expect_int ("sscanf %o", c, 63);
  // the line format doom uses for its config file
  char name[80];
  char value[100];
  expect_int ("sscanf %[",
              sscanf ("key_up   \"w s\"\n", "%79s %99[^\n]", name, value), 2);
  expect ("sscanf name", name, "key_up");
  expect ("sscanf value", value, "\"w s\"");
  expect_int ("sscanf mismatch", sscanf ("abc", "%d", &a), 0);
}

static void
check_files (void)
{
  FILE *file = fopen ("libctest.tmp", "w");
  if (!file)
    {
      printf ("libctest: fopen for writing failed\n");
      failures++;
      return;
    }
  fprintf (file, "%d %s %x\nsecond line\n", 1234, "word", 0xab);
  fclose (file);

  file = fopen ("libctest.tmp", "r");
  int num = 0, hex = 0;
  char word[16] = "";
  expect_int ("fscanf", fscanf (file, "%d %15s %x\n", &num, word, &hex), 3);
  expect_int ("fscanf num", num, 1234);
  expect ("fscanf word", word, "word");
  expect_int ("fscanf hex", hex, 0xab);
  char line[32] = "";
  fgets (line, sizeof (line), file);
  expect ("fgets", line, "second line\n");
  expect_int ("feof before", feof (file), 0);
  expect_int ("fgetc at end", fgetc (file), EOF);
  expect_int ("feof after", feof (file), 1);
  fseek (file, 0, SEEK_END);
  expect_int ("ftell end", ftell (file), 25);
  fseek (file, 5, SEEK_SET);
  expect_int ("fgetc after fseek", fgetc (file), 'w');
  expect_int ("ftell", ftell (file), 6);
  char block[4] = "";
  expect_int ("fread", fread (block, 1, 3, file), 3);
  expect ("fread data", block, "ord");
  fclose (file);

  expect_int ("rename", rename ("libctest.tmp", "libctest.tm2"), 0);
  struct stat st;
  expect_int ("stat renamed", stat ("libctest.tm2", &st), 0);
  expect_int ("stat size", st.st_size, 25);
  expect_int ("stat old name", stat ("libctest.tmp", &st), -1);
  expect_int ("stat dir", stat ("bin", &st) == 0 && S_ISDIR (st.st_mode), 1);
  expect_int ("remove", remove ("libctest.tm2"), 0);
  expect_int ("fopen missing", fopen ("libctest.tm2", "r") == 0, 1);
}

int
main (void)
{
  check_printf ();
  check_strings ();
  check_numbers ();
  check_scanf ();
  check_files ();
  printf ("libctest: %s\n", failures ? "failed" : "ok");
  return failures != 0;
}
