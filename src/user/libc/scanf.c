#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// scanf reads chars from a string or a file through this
struct scan_in
{
  const char *str; // 0 when reading the file
  FILE *file;
  int count; // chars consumed, for %n
};

static int
scan_get (struct scan_in *in)
{
  int c;
  if (in->str)
    {
      c = *in->str ? (unsigned char)*in->str++ : EOF;
    }
  else
    {
      c = fgetc (in->file);
    }
  if (c != EOF)
    {
      in->count++;
    }
  return c;
}

static void
scan_unget (struct scan_in *in, int c)
{
  if (c == EOF)
    {
      return;
    }
  in->count--;
  if (in->str)
    {
      in->str--;
    }
  else
    {
      ungetc (c, in->file);
    }
}

static int
digit_value (int c)
{
  if (isdigit (c))
    {
      return c - '0';
    }
  if (c >= 'a' && c <= 'z')
    {
      return c - 'a' + 10;
    }
  if (c >= 'A' && c <= 'Z')
    {
      return c - 'A' + 10;
    }
  return 99;
}

// reads an integer of at most width chars. base 0 detects 0x and 0
// prefixes. returns 0 if no digit was read
static int
scan_int (struct scan_in *in, int base, int width, uint64_t *out,
          int *negative)
{
  int c = scan_get (in);
  int used = 0;
  *negative = 0;
  if ((c == '-' || c == '+') && used < width)
    {
      *negative = c == '-';
      used++;
      c = scan_get (in);
    }
  if ((base == 0 || base == 16) && c == '0' && used < width)
    {
      int next = scan_get (in);
      if ((next == 'x' || next == 'X') && used + 1 < width)
        {
          base = 16;
          used += 2;
          c = scan_get (in);
        }
      else
        {
          // the 0 is a digit itself
          scan_unget (in, next);
          base = base ? base : 8;
        }
    }
  base = base ? base : 10;
  uint64_t value = 0;
  int digits = 0;
  while (used < width && digit_value (c) < base)
    {
      value = value * base + digit_value (c);
      digits++;
      used++;
      c = scan_get (in);
    }
  scan_unget (in, c);
  *out = value;
  // a lone 0 before a missing hex digit still counts
  return digits > 0 || base == 8 || base == 16;
}

static int
scan_core (struct scan_in *in, const char *fmt, va_list args)
{
  int assigned = 0;
  int c;
  for (; *fmt; fmt++)
    {
      if (isspace ((unsigned char)*fmt))
        {
          // any amount of whitespace, also none
          while (isspace (c = scan_get (in)))
            {
            }
          scan_unget (in, c);
          continue;
        }
      if (*fmt != '%' || fmt[1] == '%')
        {
          fmt += *fmt == '%';
          c = scan_get (in);
          if (c != (unsigned char)*fmt)
            {
              scan_unget (in, c);
              break;
            }
          continue;
        }
      fmt++;
      int suppress = *fmt == '*';
      fmt += suppress;
      int width = 0;
      while (isdigit ((unsigned char)*fmt))
        {
          width = width * 10 + (*fmt++ - '0');
        }
      int size = 0; // -1 char (hh), 0 int, 1 long, 2 long long, -2 short
      while (*fmt == 'h' || *fmt == 'l' || *fmt == 'z' || *fmt == 'j')
        {
          size += *fmt == 'l' ? 1 : *fmt == 'h' ? -1 : 0;
          fmt++;
        }
      char conv = *fmt;
      if (conv == 'n')
        {
          if (!suppress)
            {
              *va_arg (args, int *) = in->count;
            }
          continue;
        }
      // everything but %c and %[ skips leading whitespace
      if (conv != 'c' && conv != '[')
        {
          while (isspace (c = scan_get (in)))
            {
            }
          scan_unget (in, c);
        }
      if (conv == 'd' || conv == 'i' || conv == 'u' || conv == 'x'
          || conv == 'X' || conv == 'o')
        {
          int base = conv == 'i' ? 0
                     : conv == 'x' || conv == 'X' ? 16
                     : conv == 'o'               ? 8
                                                 : 10;
          uint64_t value;
          int negative;
          if (!scan_int (in, base, width ? width : 64, &value, &negative))
            {
              break;
            }
          if (negative)
            {
              value = -value;
            }
          if (!suppress)
            {
              if (size >= 2)
                {
                  *va_arg (args, uint64_t *) = value;
                }
              else if (size <= -2)
                {
                  *va_arg (args, char *) = value;
                }
              else if (size == -1)
                {
                  *va_arg (args, short *) = value;
                }
              else
                {
                  *va_arg (args, uint32_t *) = value;
                }
              assigned++;
            }
          continue;
        }
      if (conv == 's' || conv == 'c' || conv == '[')
        {
          // the set of chars %[ accepts, ^ inverts it
          char set[256];
          memset (set, conv != 's', sizeof (set));
          if (conv == 's')
            {
              for (int i = 0; i < 256; i++)
                {
                  set[i] = !isspace (i);
                }
            }
          else if (conv == '[')
            {
              fmt++;
              int invert = *fmt == '^';
              fmt += invert;
              memset (set, invert, sizeof (set));
              // a ] right at the start is part of the set
              do
                {
                  if (fmt[1] == '-' && fmt[2] && fmt[2] != ']')
                    {
                      for (int i = (unsigned char)fmt[0];
                           i <= (unsigned char)fmt[2]; i++)
                        {
                          set[i] = !invert;
                        }
                      fmt += 3;
                    }
                  else
                    {
                      set[(unsigned char)*fmt++] = !invert;
                    }
                }
              while (*fmt && *fmt != ']');
            }
          int max = width ? width : conv == 'c' ? 1 : 0x7FFFFFFF;
          char *out = suppress ? 0 : va_arg (args, char *);
          int len = 0;
          while (len < max && (c = scan_get (in)) != EOF
                 && set[(unsigned char)c])
            {
              if (out)
                {
                  out[len] = c;
                }
              len++;
            }
          if (len < max)
            {
              scan_unget (in, c);
            }
          if (len == 0 || (conv == 'c' && len < max))
            {
              break;
            }
          if (out)
            {
              if (conv != 'c')
                {
                  out[len] = '\0';
                }
              assigned++;
            }
          continue;
        }
      // unknown conversion
      break;
    }
  return assigned;
}

int
vsscanf (const char *str, const char *fmt, va_list args)
{
  struct scan_in in = { str, 0, 0 };
  if (!*str)
    {
      return EOF;
    }
  return scan_core (&in, fmt, args);
}

int
sscanf (const char *str, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  int res = vsscanf (str, fmt, args);
  va_end (args);
  return res;
}

int
vfscanf (FILE *file, const char *fmt, va_list args)
{
  struct scan_in in = { 0, file, 0 };
  int c = fgetc (file);
  if (c == EOF)
    {
      return EOF;
    }
  ungetc (c, file);
  return scan_core (&in, fmt, args);
}

int
fscanf (FILE *file, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  int res = vfscanf (file, fmt, args);
  va_end (args);
  return res;
}
