#include "ctype.h"

// all predicates return non-zero when the char matches and zero otherwise

int32_t
isalnum (int32_t c)
{
  return isdigit (c) || isalpha (c);
}

int32_t
isalpha (int32_t c)
{
  return isupper (c) || islower (c);
}

int32_t
iscntrl (int32_t c)
{
  // ascii control chars
  return (c >= 0 && c <= 31) || c == 127;
}

int32_t
isdigit (int32_t c)
{
  return c >= '0' && c <= '9';
}

int32_t
isgraph (int32_t c)
{
  return c >= 33 && c <= 126;
}

int32_t
islower (int32_t c)
{
  return c >= 'a' && c <= 'z';
}

int32_t
isprint (int32_t c)
{
  // printable chars are graphical chars plus space
  return c >= 32 && c <= 126;
}

int32_t
ispunct (int32_t c)
{
  return isgraph (c) && !isalnum (c);
}

int32_t
isspace (int32_t c)
{
  // space, \t, \n, \v, \f, \r
  return c == ' ' || (c >= 9 && c <= 13);
}

int32_t
isupper (int32_t c)
{
  return c >= 'A' && c <= 'Z';
}

int32_t
isxdigit (int32_t c)
{
  return isdigit (c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int32_t
tolower (int32_t c)
{
  if (isupper (c))
    {
      return c + 32;
    }
  return c;
}

int32_t
toupper (int32_t c)
{
  if (islower (c))
    {
      return c - 32;
    }
  return c;
}
