#include "moos.h"

void
print_int (int32_t num)
{
  char digits[10];
  uint32_t idx = 0;
  uint32_t mag = (uint32_t)num;
  if (num < 0)
    {
      putchar ('-');
      mag = -mag;
    }
  do
    {
      digits[idx++] = '0' + (mag % 10);
      mag /= 10;
    }
  while (mag > 0);
  while (idx > 0)
    {
      putchar (digits[--idx]);
    }
}

uint32_t
readline (char *buf, uint32_t max)
{
  // input from a file or pipe isnt typed, so it isnt echoed either
  int echo = isatty (STDIN);
  uint32_t len = 0;
  while (1)
    {
      int c = getchar ();
      // -1 is the end of the input, 4 is ctrl+d typed on the terminal
      if (c < 0 || c == '\n' || (echo && c == 4))
        {
          if (echo)
            {
              putchar ('\n');
            }
          break;
        }
      if (c == '\b')
        {
          if (len > 0)
            {
              len--;
              if (echo)
                {
                  putchar ('\b');
                }
            }
          continue;
        }
      if (c >= 32 && c <= 126 && len < max - 1)
        {
          buf[len++] = c;
          if (echo)
            {
              putchar (c);
            }
        }
    }
  buf[len] = '\0';
  return len;
}

static void
busy_loops (uint32_t count)
{
  for (volatile uint32_t i = 0; i < count; i++)
    {
    }
}

uint32_t
busy_calibrate (void)
{
  // start right after a tick so the window is exactly 100 ms long
  uint32_t start = uptime ();
  while (uptime () == start)
    {
    }
  start = uptime ();
  uint32_t loops = 0;
  while (uptime () - start < 100)
    {
      busy_loops (10000);
      loops += 10000;
    }
  return loops / 100;
}

void
busy_ms (uint32_t ms, uint32_t loops_per_ms)
{
  for (uint32_t i = 0; i < ms; i++)
    {
      busy_loops (loops_per_ms);
    }
}

uint32_t
strlen (const char *str)
{
  uint32_t len = 0;
  while (str[len])
    {
      len++;
    }
  return len;
}

int
strcmp (const char *a, const char *b)
{
  while (*a && *a == *b)
    {
      a++;
      b++;
    }
  return (unsigned char)*a - (unsigned char)*b;
}

void *
memcpy (void *dest, const void *src, uint32_t len)
{
  uint8_t *d = dest;
  const uint8_t *s = src;
  while (len--)
    {
      *d++ = *s++;
    }
  return dest;
}

void *
memset (void *dest, int c, uint32_t len)
{
  uint8_t *d = dest;
  while (len--)
    {
      *d++ = c;
    }
  return dest;
}

int
strncmp (const char *a, const char *b, uint32_t n)
{
  for (; n > 0; n--, a++, b++)
    {
      if (*a != *b || !*a)
        {
          return (unsigned char)*a - (unsigned char)*b;
        }
    }
  return 0;
}

int
strncasecmp (const char *a, const char *b, uint32_t n)
{
  for (; n > 0; n--, a++, b++)
    {
      int diff = tolower ((unsigned char)*a) - tolower ((unsigned char)*b);
      if (diff || !*a)
        {
          return diff;
        }
    }
  return 0;
}

char *
strcpy (char *dest, const char *src)
{
  memcpy (dest, src, strlen (src) + 1);
  return dest;
}

char *
strcat (char *dest, const char *src)
{
  strcpy (dest + strlen (dest), src);
  return dest;
}

char *
strchr (const char *str, int c)
{
  for (; *str; str++)
    {
      if (*str == (char)c)
        {
          return (char *)str;
        }
    }
  return c ? 0 : (char *)str;
}

void *
memmove (void *dest, const void *src, uint32_t len)
{
  uint8_t *d = dest;
  const uint8_t *s = src;
  if (d < s)
    {
      return memcpy (dest, src, len);
    }
  while (len--)
    {
      d[len] = s[len];
    }
  return dest;
}

int
memcmp (const void *a, const void *b, uint32_t len)
{
  const uint8_t *x = a;
  const uint8_t *y = b;
  for (uint32_t i = 0; i < len; i++)
    {
      if (x[i] != y[i])
        {
          return x[i] - y[i];
        }
    }
  return 0;
}

int
atoi (const char *str)
{
  int sign = 1;
  int num = 0;
  while (isspace (*str))
    {
      str++;
    }
  if (*str == '-' || *str == '+')
    {
      sign = *str++ == '-' ? -1 : 1;
    }
  while (isdigit (*str))
    {
      num = num * 10 + (*str++ - '0');
    }
  return sign * num;
}

int
isdigit (int c)
{
  return c >= '0' && c <= '9';
}

int
isalpha (int c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int
isspace (int c)
{
  return c == ' ' || (c >= '\t' && c <= '\r');
}

int
isprint (int c)
{
  return c >= 32 && c <= 126;
}

int
tolower (int c)
{
  return c >= 'A' && c <= 'Z' ? c + 'a' - 'A' : c;
}

int
toupper (int c)
{
  return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
}

// output of vsnprintf, counts every char but only stores what fits
struct format_out
{
  char *buf;
  uint32_t size;
  uint32_t len;
};

static void
format_put (struct format_out *out, char c)
{
  if (out->len + 1 < out->size)
    {
      out->buf[out->len] = c;
    }
  out->len++;
}

// pads str (len chars) to width. zero padding goes after a sign
static void
format_padded (struct format_out *out, const char *str, uint32_t len,
               uint32_t width, int left, char pad)
{
  if (pad == '0' && len > 0 && (str[0] == '-' || str[0] == '+'))
    {
      format_put (out, *str++);
      len--;
      width = width ? width - 1 : 0;
    }
  if (!left)
    {
      for (uint32_t i = len; i < width; i++)
        {
          format_put (out, pad);
        }
    }
  for (uint32_t i = 0; i < len; i++)
    {
      format_put (out, str[i]);
    }
  if (left)
    {
      for (uint32_t i = len; i < width; i++)
        {
          format_put (out, ' ');
        }
    }
}

// writes num in base to digits (at least min_digits of them) and returns
// how many chars it wrote
static uint32_t
format_number (char *digits, uint64_t num, uint32_t base, int upper,
               int negative, int plus, uint32_t min_digits)
{
  const char *set = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  char reversed[24];
  uint32_t count = 0;
  uint32_t len = 0;
  // 32 bit numbers dont need the slow 64 bit division
  while (num > 0xFFFFFFFFu)
    {
      reversed[count++] = set[num % base];
      num /= base;
    }
  uint32_t small = num;
  while (small || count < min_digits || count == 0)
    {
      reversed[count++] = set[small % base];
      small /= base;
    }
  if (negative)
    {
      digits[len++] = '-';
    }
  else if (plus)
    {
      digits[len++] = '+';
    }
  while (count)
    {
      digits[len++] = reversed[--count];
    }
  return len;
}

// fixed point float, rounded to precision digits
static uint32_t
format_float (char *digits, double value, uint32_t precision, int plus)
{
  uint32_t len = 0;
  if (value != value)
    {
      memcpy (digits, "nan", 3);
      return 3;
    }
  if (value < 0)
    {
      digits[len++] = '-';
      value = -value;
    }
  else if (plus)
    {
      digits[len++] = '+';
    }
  if (precision > 9)
    {
      precision = 9;
    }
  uint32_t scale = 1;
  for (uint32_t i = 0; i < precision; i++)
    {
      scale *= 10;
    }
  if (value > 1e18)
    {
      memcpy (digits + len, "inf", 3);
      return len + 3;
    }
  uint64_t whole = (uint64_t)value;
  uint32_t fraction = (uint32_t)((value - whole) * scale + 0.5);
  if (fraction >= scale)
    {
      whole++;
      fraction -= scale;
    }
  len += format_number (digits + len, whole, 10, 0, 0, 0, 1);
  if (precision)
    {
      digits[len++] = '.';
      len += format_number (digits + len, fraction, 10, 0, 0, 0, precision);
    }
  return len;
}

int
vsnprintf (char *buf, uint32_t size, const char *fmt, va_list args)
{
  struct format_out out = { buf, size, 0 };
  for (; *fmt; fmt++)
    {
      if (*fmt != '%')
        {
          format_put (&out, *fmt);
          continue;
        }
      fmt++;
      int left = 0;
      int plus = 0;
      char pad = ' ';
      for (;; fmt++)
        {
          if (*fmt == '-')
            {
              left = 1;
            }
          else if (*fmt == '+')
            {
              plus = 1;
            }
          else if (*fmt == '0')
            {
              pad = '0';
            }
          else if (*fmt != ' ' && *fmt != '#')
            {
              break;
            }
        }
      uint32_t width = 0;
      if (*fmt == '*')
        {
          int arg = va_arg (args, int);
          left |= arg < 0;
          width = arg < 0 ? -arg : arg;
          fmt++;
        }
      while (isdigit (*fmt))
        {
          width = width * 10 + (*fmt++ - '0');
        }
      int precision = -1;
      if (*fmt == '.')
        {
          fmt++;
          precision = 0;
          if (*fmt == '*')
            {
              precision = va_arg (args, int);
              fmt++;
            }
          while (isdigit (*fmt))
            {
              precision = precision * 10 + (*fmt++ - '0');
            }
        }
      // length modifiers: only long long changes the size of the argument
      int longs = 0;
      while (*fmt == 'l' || *fmt == 'h' || *fmt == 'z' || *fmt == 'j'
             || *fmt == 't')
        {
          longs += *fmt++ == 'l';
        }
      char digits[48];
      uint32_t len = 0;
      switch (*fmt)
        {
        case 's':
          {
            const char *str = va_arg (args, const char *);
            str = str ? str : "(null)";
            uint32_t str_len = strlen (str);
            if (precision >= 0 && (uint32_t)precision < str_len)
              {
                str_len = precision;
              }
            format_padded (&out, str, str_len, width, left, ' ');
            continue;
          }
        case 'c':
          digits[len++] = (char)va_arg (args, int);
          pad = ' ';
          break;
        case 'd':
        case 'i':
          {
            int64_t num = longs >= 2 ? va_arg (args, int64_t)
                                     : va_arg (args, int32_t);
            uint64_t mag = num < 0 ? -(uint64_t)num : (uint64_t)num;
            len = format_number (digits, mag, 10, 0, num < 0, plus,
                                 precision > 0 ? precision : 1);
            break;
          }
        case 'u':
        case 'x':
        case 'X':
        case 'o':
        case 'p':
          {
            uint64_t num = longs >= 2 ? va_arg (args, uint64_t)
                                      : va_arg (args, uint32_t);
            uint32_t base = *fmt == 'u' ? 10 : *fmt == 'o' ? 8 : 16;
            if (*fmt == 'p')
              {
                digits[len++] = '0';
                digits[len++] = 'x';
              }
            len += format_number (digits + len, num, base, *fmt == 'X', 0, 0,
                                  precision > 0 ? precision : 1);
            break;
          }
        case 'f':
        case 'F':
        case 'g':
        case 'e':
          len = format_float (digits, va_arg (args, double),
                              precision >= 0 ? precision : 6, plus);
          break;
        case '%':
          digits[len++] = '%';
          break;
        default:
          // unknown conversion, print it as it is
          digits[len++] = '%';
          if (*fmt)
            {
              digits[len++] = *fmt;
            }
          else
            {
              fmt--;
            }
          break;
        }
      format_padded (&out, digits, len, width, left, pad);
    }
  if (size > 0)
    {
      buf[out.len < size ? out.len : size - 1] = '\0';
    }
  return out.len;
}

int
snprintf (char *buf, uint32_t size, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  int len = vsnprintf (buf, size, fmt, args);
  va_end (args);
  return len;
}

int
vprintf (const char *fmt, va_list args)
{
  char buf[512];
  int len = vsnprintf (buf, sizeof (buf), fmt, args);
  write (STDOUT, buf, len < (int)sizeof (buf) ? len : sizeof (buf) - 1);
  return len;
}

int
printf (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  int len = vprintf (fmt, args);
  va_end (args);
  return len;
}

void
eprint (const char *str)
{
  write (STDERR, str, strlen (str));
}

void
print_padded (const char *str, uint32_t width)
{
  print (str);
  for (uint32_t len = strlen (str); len < width; len++)
    {
      putchar (' ');
    }
}

void
print_error (const char *what, int err)
{
  // error numbers from src/kernel/config.h
  static const char *reasons[] = {
    "error",
    "io error",
    "out of memory",
    "invalid argument",
    "invalid path",
    "too many open files",
    "invalid disk",
    "bad signature",
    "not permitted",
    "not implemented",
    "not a directory",
    "not found",
    "directory not empty",
    "already exists",
    "broken pipe",
  };
  int code = -err;
  eprint (what);
  eprint (": ");
  eprint (code > 0 && code < (int)(sizeof (reasons) / sizeof (reasons[0]))
              ? reasons[code]
              : "error");
  eprint ("\n");
}

// first fit allocator on top of sbrk. blocks are kept in address order so
// neighbouring free blocks can be merged
struct block
{
  uint32_t size; // payload bytes after the header
  uint32_t free;
  struct block *next;
};

#define ALIGN 8
#define HEADER_SIZE ((sizeof (struct block) + ALIGN - 1) & ~(ALIGN - 1))
#define MIN_SPLIT (HEADER_SIZE + ALIGN)

static struct block *heap_head = 0;
static struct block *heap_tail = 0;

static void *
block_payload (struct block *block)
{
  return (uint8_t *)block + HEADER_SIZE;
}

static struct block *
payload_block (void *ptr)
{
  return (struct block *)((uint8_t *)ptr - HEADER_SIZE);
}

void *
malloc (uint32_t size)
{
  if (size == 0)
    {
      return 0;
    }
  size = (size + ALIGN - 1) & ~(ALIGN - 1);
  for (struct block *block = heap_head; block; block = block->next)
    {
      if (!block->free || block->size < size)
        {
          continue;
        }
      // split off the unused tail if it is big enough to be useful
      if (block->size >= size + MIN_SPLIT)
        {
          struct block *rest
              = (struct block *)((uint8_t *)block_payload (block) + size);
          rest->size = block->size - size - HEADER_SIZE;
          rest->free = 1;
          rest->next = block->next;
          block->next = rest;
          block->size = size;
          if (heap_tail == block)
            {
              heap_tail = rest;
            }
        }
      block->free = 0;
      return block_payload (block);
    }
  struct block *block = sbrk (HEADER_SIZE + size);
  if (!block)
    {
      return 0;
    }
  block->size = size;
  block->free = 0;
  block->next = 0;
  if (heap_tail)
    {
      heap_tail->next = block;
    }
  else
    {
      heap_head = block;
    }
  heap_tail = block;
  return block_payload (block);
}

void
free (void *ptr)
{
  if (!ptr)
    {
      return;
    }
  payload_block (ptr)->free = 1;
  // merge runs of adjacent free blocks
  for (struct block *block = heap_head; block; block = block->next)
    {
      while (block->free && block->next && block->next->free)
        {
          if (heap_tail == block->next)
            {
              heap_tail = block;
            }
          block->size += HEADER_SIZE + block->next->size;
          block->next = block->next->next;
        }
    }
}

void *
calloc (uint32_t count, uint32_t size)
{
  uint32_t total = count * size;
  if (size && total / size != count)
    {
      return 0;
    }
  void *ptr = malloc (total);
  if (ptr)
    {
      memset (ptr, 0, total);
    }
  return ptr;
}

void *
realloc (void *ptr, uint32_t size)
{
  if (!ptr)
    {
      return malloc (size);
    }
  struct block *block = payload_block (ptr);
  if (block->size >= size)
    {
      return ptr;
    }
  void *bigger = malloc (size);
  if (bigger)
    {
      memcpy (bigger, ptr, block->size);
      free (ptr);
    }
  return bigger;
}
