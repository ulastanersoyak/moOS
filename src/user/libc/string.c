#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// memcpy, memmove, memset, memcmp, strlen, strcmp, strncmp, strcpy, strcat,
// strchr, strncasecmp and the basic ctype functions come from src/user/lib

void *
memchr (const void *ptr, int c, size_t len)
{
  const unsigned char *p = ptr;
  for (size_t i = 0; i < len; i++)
    {
      if (p[i] == (unsigned char)c)
        {
          return (void *)(p + i);
        }
    }
  return 0;
}

size_t
strnlen (const char *str, size_t max)
{
  size_t len = 0;
  while (len < max && str[len])
    {
      len++;
    }
  return len;
}

char *
strncpy (char *dest, const char *src, size_t n)
{
  size_t i = 0;
  for (; i < n && src[i]; i++)
    {
      dest[i] = src[i];
    }
  // pads with nulls, doesnt terminate when src is n chars or longer
  for (; i < n; i++)
    {
      dest[i] = '\0';
    }
  return dest;
}

char *
strncat (char *dest, const char *src, size_t n)
{
  char *end = dest + strlen (dest);
  size_t i = 0;
  for (; i < n && src[i]; i++)
    {
      end[i] = src[i];
    }
  end[i] = '\0';
  return dest;
}

char *
strrchr (const char *str, int c)
{
  const char *last = 0;
  do
    {
      if (*str == (char)c)
        {
          last = str;
        }
    }
  while (*str++);
  return (char *)last;
}

char *
strstr (const char *haystack, const char *needle)
{
  size_t len = strlen (needle);
  for (; *haystack; haystack++)
    {
      if (strncmp (haystack, needle, len) == 0)
        {
          return (char *)haystack;
        }
    }
  return len == 0 ? (char *)haystack : 0;
}

size_t
strspn (const char *str, const char *accept)
{
  size_t len = 0;
  while (str[len] && strchr (accept, str[len]))
    {
      len++;
    }
  return len;
}

size_t
strcspn (const char *str, const char *reject)
{
  size_t len = 0;
  while (str[len] && !strchr (reject, str[len]))
    {
      len++;
    }
  return len;
}

char *
strpbrk (const char *str, const char *accept)
{
  str += strcspn (str, accept);
  return *str ? (char *)str : 0;
}

char *
strtok (char *str, const char *delim)
{
  static char *next = 0;
  if (!str)
    {
      str = next;
    }
  if (!str)
    {
      return 0;
    }
  str += strspn (str, delim);
  if (!*str)
    {
      next = 0;
      return 0;
    }
  char *end = str + strcspn (str, delim);
  next = *end ? end + 1 : 0;
  *end = '\0';
  return str;
}

char *
strdup (const char *str)
{
  return strndup (str, strlen (str));
}

char *
strndup (const char *str, size_t max)
{
  size_t len = strnlen (str, max);
  char *copy = malloc (len + 1);
  if (copy)
    {
      memcpy (copy, str, len);
      copy[len] = '\0';
    }
  return copy;
}

char *
strerror (int err)
{
  switch (err)
    {
    case 0:
      return "no error";
    case ENOENT:
      return "no such file or directory";
    case ENOMEM:
      return "out of memory";
    case EINVAL:
      return "invalid argument";
    case EEXIST:
      return "file exists";
    case ENOSYS:
      return "not implemented";
    default:
      return "error";
    }
}

int
strcasecmp (const char *a, const char *b)
{
  return strncasecmp (a, b, (size_t)-1);
}

int
isalnum (int c)
{
  return isalpha (c) || isdigit (c);
}

int
iscntrl (int c)
{
  return (c >= 0 && c < 32) || c == 127;
}

int
isgraph (int c)
{
  return c > 32 && c < 127;
}

int
islower (int c)
{
  return c >= 'a' && c <= 'z';
}

int
isupper (int c)
{
  return c >= 'A' && c <= 'Z';
}

int
ispunct (int c)
{
  return isgraph (c) && !isalnum (c);
}

int
isxdigit (int c)
{
  return isdigit (c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
