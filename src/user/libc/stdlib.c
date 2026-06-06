#include "sys.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

// malloc, calloc, realloc, free, atoi and exit come from src/user/lib

int errno = 0;

void
abort (void)
{
  fputs ("abort\n", stderr);
  // like a program killed by a signal
  exit (134);
}

void
__assert_fail (const char *expr, const char *file, int line)
{
  fprintf (stderr, "%s:%d: assertion failed: %s\n", file, line, expr);
  abort ();
}

long
atol (const char *str)
{
  return strtol (str, 0, 10);
}

double
atof (const char *str)
{
  return strtod (str, 0);
}

static int
digit_value (int c)
{
  if (isdigit (c))
    {
      return c - '0';
    }
  if (isalpha (c))
    {
      return tolower (c) - 'a' + 10;
    }
  return 99;
}

// parses the sign, base prefix and digits shared by the strto functions
static uint64_t
parse_integer (const char *str, char **end, int base, int *negative)
{
  const char *start = str;
  while (isspace ((unsigned char)*str))
    {
      str++;
    }
  *negative = *str == '-';
  if (*str == '-' || *str == '+')
    {
      str++;
    }
  if ((base == 0 || base == 16) && str[0] == '0'
      && (str[1] == 'x' || str[1] == 'X') && digit_value (str[2]) < 16)
    {
      base = 16;
      str += 2;
    }
  else if (base == 0)
    {
      base = str[0] == '0' ? 8 : 10;
    }
  uint64_t value = 0;
  const char *digits = str;
  while (digit_value ((unsigned char)*str) < base)
    {
      value = value * base + digit_value ((unsigned char)*str++);
    }
  if (end)
    {
      *end = (char *)(str == digits ? start : str);
    }
  return value;
}

long
strtol (const char *str, char **end, int base)
{
  int negative;
  uint64_t value = parse_integer (str, end, base, &negative);
  return negative ? -(long)value : (long)value;
}

unsigned long
strtoul (const char *str, char **end, int base)
{
  int negative;
  uint64_t value = parse_integer (str, end, base, &negative);
  return negative ? -(unsigned long)value : (unsigned long)value;
}

long long
strtoll (const char *str, char **end, int base)
{
  int negative;
  uint64_t value = parse_integer (str, end, base, &negative);
  return negative ? -(long long)value : (long long)value;
}

unsigned long long
strtoull (const char *str, char **end, int base)
{
  int negative;
  uint64_t value = parse_integer (str, end, base, &negative);
  return negative ? -(unsigned long long)value : (unsigned long long)value;
}

// decimal numbers with an optional fraction and exponent
double
strtod (const char *str, char **end)
{
  const char *start = str;
  while (isspace ((unsigned char)*str))
    {
      str++;
    }
  double sign = 1;
  if (*str == '-' || *str == '+')
    {
      sign = *str++ == '-' ? -1 : 1;
    }
  double value = 0;
  int digits = 0;
  while (isdigit ((unsigned char)*str))
    {
      value = value * 10 + (*str++ - '0');
      digits++;
    }
  if (*str == '.')
    {
      str++;
      double scale = 0.1;
      while (isdigit ((unsigned char)*str))
        {
          value += (*str++ - '0') * scale;
          scale /= 10;
          digits++;
        }
    }
  if (digits && (*str == 'e' || *str == 'E'))
    {
      char *exp_end;
      long exp = strtol (str + 1, &exp_end, 10);
      if (exp_end != str + 1)
        {
          str = exp_end;
          for (; exp > 0; exp--)
            {
              value *= 10;
            }
          for (; exp < 0; exp++)
            {
              value /= 10;
            }
        }
    }
  if (end)
    {
      *end = (char *)(digits ? str : start);
    }
  return sign * value;
}

float
strtof (const char *str, char **end)
{
  return strtod (str, end);
}

long double
strtold (const char *str, char **end)
{
  return strtod (str, end);
}

int
abs (int num)
{
  return num < 0 ? -num : num;
}

long
labs (long num)
{
  return num < 0 ? -num : num;
}

static uint32_t rand_state = 1;

int
rand (void)
{
  rand_state = rand_state * 1103515245 + 12345;
  return (rand_state >> 16) & RAND_MAX;
}

void
srand (unsigned int seed)
{
  rand_state = seed;
}

static void
swap_bytes (uint8_t *a, uint8_t *b, size_t size)
{
  for (size_t i = 0; i < size; i++)
    {
      uint8_t tmp = a[i];
      a[i] = b[i];
      b[i] = tmp;
    }
}

// shell sort, no recursion and no extra memory
void
qsort (void *base, size_t count, size_t size,
       int (*compare) (const void *, const void *))
{
  uint8_t *items = base;
  for (size_t gap = count / 2; gap > 0; gap /= 2)
    {
      for (size_t i = gap; i < count; i++)
        {
          for (size_t j = i; j >= gap
                             && compare (items + (j - gap) * size,
                                         items + j * size)
                                    > 0;
               j -= gap)
            {
              swap_bytes (items + (j - gap) * size, items + j * size, size);
            }
        }
    }
}

void *
bsearch (const void *key, const void *base, size_t count, size_t size,
         int (*compare) (const void *, const void *))
{
  const uint8_t *items = base;
  size_t low = 0;
  size_t high = count;
  while (low < high)
    {
      size_t mid = (low + high) / 2;
      int res = compare (key, items + mid * size);
      if (res == 0)
        {
          return (void *)(items + mid * size);
        }
      if (res < 0)
        {
          high = mid;
        }
      else
        {
          low = mid + 1;
        }
    }
  return 0;
}

char *
getenv (const char *name)
{
  return 0;
}

int
system (const char *command)
{
  return -1;
}

int
stat (const char *path, struct stat *out)
{
  // listing only works for directories, open works for both
  struct moos_dirent entry;
  if (listdir (path, &entry, 1) >= 0)
    {
      out->st_mode = S_IFDIR;
      out->st_size = 0;
      return 0;
    }
  int fd = moos_open (path, MOOS_READ);
  if (fd >= 0)
    {
      out->st_mode = S_IFREG;
      out->st_size = filesize (fd);
      close (fd);
      return 0;
    }
  errno = ENOENT;
  return -1;
}

int
access (const char *path, int mode)
{
  struct stat st;
  return stat (path, &st);
}

int
unlink (const char *path)
{
  return remove (path) < 0 ? -1 : 0;
}

int
rmdir (const char *path)
{
  return remove (path) < 0 ? -1 : 0;
}

off_t
lseek (int fd, off_t offset, int whence)
{
  // the kernel doesnt track positions for us, only absolute seeks work
  if (whence == SEEK_END)
    {
      offset += filesize (fd);
    }
  else if (whence != SEEK_SET)
    {
      errno = ENOSYS;
      return -1;
    }
  return seek (fd, offset, SEEK_SET) < 0 ? -1 : offset;
}

int
usleep (unsigned int usec)
{
  sleep (usec / 1000);
  return 0;
}

int
gettimeofday (struct timeval *tv, void *tz)
{
  uint32_t ms = uptime ();
  tv->tv_sec = ms / 1000;
  tv->tv_usec = (ms % 1000) * 1000;
  return 0;
}

int
open (const char *path, int flags, ...)
{
  int mode;
  if ((flags & 3) == O_RDONLY)
    {
      mode = MOOS_READ;
    }
  else if ((flags & 3) == O_WRONLY)
    {
      mode = flags & O_APPEND ? MOOS_APPEND : MOOS_WRITE;
    }
  else
    {
      errno = EINVAL;
      return -1;
    }
  int fd = moos_open (path, mode);
  if (fd < 0)
    {
      errno = ENOENT;
      return -1;
    }
  return fd;
}

char *
getcwd (char *buf, size_t len)
{
  if (moos_getcwd (buf, len) < 0)
    {
      errno = ERANGE;
      return 0;
    }
  return buf;
}

char *
realpath (const char *path, char *out)
{
  struct stat st;
  if (stat (path, &st) < 0)
    {
      return 0;
    }
  // the working directory always starts with the drive, "0:/"
  char joined[2 * FILENAME_MAX + 2];
  if (isdigit ((unsigned char)path[0]) && path[1] == ':')
    {
      joined[0] = 0;
    }
  else if (!getcwd (joined, FILENAME_MAX + 1))
    {
      return 0;
    }
  else if (path[0] == '/')
    {
      joined[2] = 0; // keep the drive
    }
  else
    {
      strcat (joined, "/");
    }
  if (strlen (joined) + strlen (path) > 2 * FILENAME_MAX)
    {
      errno = ERANGE;
      return 0;
    }
  strcat (joined, path);

  char *res = out ? out : malloc (FILENAME_MAX + 1);
  if (!res)
    {
      errno = ENOMEM;
      return 0;
    }
  // like the kernel does it: the drive, then every part that isnt empty or
  // ".", ".." removes the part before it
  int len = 2;
  memcpy (res, joined, 2);
  const char *part = joined + 2;
  while (*part)
    {
      while (*part == '/')
        {
          part++;
        }
      const char *part_end = part;
      while (*part_end && *part_end != '/')
        {
          part_end++;
        }
      int part_len = part_end - part;
      if (part_len == 2 && part[0] == '.' && part[1] == '.')
        {
          while (len > 2 && res[len - 1] != '/')
            {
              len--;
            }
          if (len > 2)
            {
              len--;
            }
        }
      else if (part_len > 0 && !(part_len == 1 && part[0] == '.'))
        {
          if (len + 1 + part_len > FILENAME_MAX)
            {
              if (!out)
                {
                  free (res);
                }
              errno = ERANGE;
              return 0;
            }
          res[len++] = '/';
          memcpy (res + len, part, part_len);
          len += part_len;
        }
      part = part_end;
    }
  if (len == 2)
    {
      res[len++] = '/';
    }
  res[len] = 0;
  return res;
}

int
execvp (const char *file, char *const argv[])
{
  int pid = spawn (file, (char **)argv, 0);
  if (pid < 0)
    {
      errno = ENOENT;
      return -1;
    }
  exit (wait (pid));
}
