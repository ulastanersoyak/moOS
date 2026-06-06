#include "sys.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct _FILE
{
  int fd;
  long pos; // the kernel doesnt report the position, it is counted here
  int eof;
  int error;
  int unget; // char pushed back by ungetc, -1 if none
};

static FILE std_files[3] = {
  { 0, 0, 0, 0, -1 },
  { 1, 0, 0, 0, -1 },
  { 2, 0, 0, 0, -1 },
};

FILE *stdin = &std_files[0];
FILE *stdout = &std_files[1];
FILE *stderr = &std_files[2];

FILE *
fopen (const char *path, const char *mode)
{
  int moos_mode;
  switch (mode[0])
    {
    case 'r':
      moos_mode = MOOS_READ;
      break;
    case 'w':
      moos_mode = MOOS_WRITE;
      break;
    case 'a':
      moos_mode = MOOS_APPEND;
      break;
    default:
      errno = EINVAL;
      return 0;
    }
  int fd = moos_open (path, moos_mode);
  if (fd < 0)
    {
      errno = ENOENT;
      return 0;
    }
  FILE *file = calloc (1, sizeof (FILE));
  if (!file)
    {
      close (fd);
      errno = ENOMEM;
      return 0;
    }
  file->fd = fd;
  file->unget = -1;
  if (moos_mode == MOOS_APPEND)
    {
      int size = filesize (fd);
      file->pos = size > 0 ? size : 0;
    }
  return file;
}

FILE *
fdopen (int fd, const char *mode)
{
  FILE *file = calloc (1, sizeof (FILE));
  if (!file)
    {
      errno = ENOMEM;
      return 0;
    }
  file->fd = fd;
  file->unget = -1;
  if (mode[0] == 'a')
    {
      int size = filesize (fd);
      file->pos = size > 0 ? size : 0;
    }
  return file;
}

int
fclose (FILE *file)
{
  int res = close (file->fd);
  if (file < std_files || file >= std_files + 3)
    {
      free (file);
    }
  return res < 0 ? EOF : 0;
}

size_t
fread (void *buf, size_t size, size_t count, FILE *file)
{
  size_t total = size * count;
  if (total == 0)
    {
      return 0;
    }
  uint8_t *out = buf;
  size_t got = 0;
  if (file->unget >= 0)
    {
      out[got++] = file->unget;
      file->unget = -1;
    }
  while (got < total)
    {
      int res = read (file->fd, out + got, total - got);
      if (res < 0)
        {
          file->error = 1;
          break;
        }
      if (res == 0)
        {
          file->eof = 1;
          break;
        }
      got += res;
    }
  file->pos += got;
  return got / size;
}

size_t
fwrite (const void *buf, size_t size, size_t count, FILE *file)
{
  size_t total = size * count;
  if (total == 0)
    {
      return 0;
    }
  int res = write (file->fd, buf, total);
  if (res < 0)
    {
      file->error = 1;
      return 0;
    }
  file->pos += res;
  return res / size;
}

int
fseek (FILE *file, long offset, int whence)
{
  long pos = offset;
  if (whence == SEEK_CUR)
    {
      pos = file->pos + offset;
    }
  else if (whence == SEEK_END)
    {
      int size = filesize (file->fd);
      if (size < 0)
        {
          errno = EBADF;
          return -1;
        }
      pos = size + offset;
    }
  if (pos < 0 || seek (file->fd, pos, SEEK_SET) < 0)
    {
      errno = EINVAL;
      return -1;
    }
  file->pos = pos;
  file->eof = 0;
  file->unget = -1;
  return 0;
}

long
ftell (FILE *file)
{
  return file->unget >= 0 ? file->pos - 1 : file->pos;
}

void
rewind (FILE *file)
{
  fseek (file, 0, SEEK_SET);
  file->error = 0;
}

int
feof (FILE *file)
{
  return file->eof;
}

int
ferror (FILE *file)
{
  return file->error;
}

void
clearerr (FILE *file)
{
  file->eof = 0;
  file->error = 0;
}

int
fflush (FILE *file)
{
  // nothing is buffered
  return 0;
}

int
fgetc (FILE *file)
{
  unsigned char c;
  return fread (&c, 1, 1, file) == 1 ? c : EOF;
}

int
getc (FILE *file)
{
  return fgetc (file);
}

int
ungetc (int c, FILE *file)
{
  if (c == EOF || file->unget >= 0)
    {
      return EOF;
    }
  file->unget = (unsigned char)c;
  file->pos--;
  file->eof = 0;
  return c;
}

char *
fgets (char *buf, int size, FILE *file)
{
  int len = 0;
  while (len < size - 1)
    {
      int c = fgetc (file);
      if (c == EOF)
        {
          break;
        }
      buf[len++] = c;
      if (c == '\n')
        {
          break;
        }
    }
  if (len == 0)
    {
      return 0;
    }
  buf[len] = '\0';
  return buf;
}

int
fputc (int c, FILE *file)
{
  unsigned char byte = c;
  return fwrite (&byte, 1, 1, file) == 1 ? byte : EOF;
}

int
putc (int c, FILE *file)
{
  return fputc (c, file);
}

int
fputs (const char *str, FILE *file)
{
  size_t len = strlen (str);
  return fwrite (str, 1, len, file) == len ? 0 : EOF;
}

int
puts (const char *str)
{
  return fputs (str, stdout) == EOF || fputc ('\n', stdout) == EOF ? EOF
                                                                    : 0;
}

int
vfprintf (FILE *file, const char *fmt, va_list args)
{
  char small[512];
  va_list copy;
  va_copy (copy, args);
  int len = vsnprintf (small, sizeof (small), fmt, args);
  char *buf = small;
  if (len >= (int)sizeof (small) && (buf = malloc (len + 1)))
    {
      vsnprintf (buf, len + 1, fmt, copy);
    }
  va_end (copy);
  if (!buf)
    {
      buf = small;
      len = sizeof (small) - 1;
    }
  fwrite (buf, 1, len, file);
  if (buf != small)
    {
      free (buf);
    }
  return len;
}

int
fprintf (FILE *file, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  int len = vfprintf (file, fmt, args);
  va_end (args);
  return len;
}

int
vsprintf (char *buf, const char *fmt, va_list args)
{
  // the caller promises the buffer is big enough
  return vsnprintf (buf, 0x7FFFFFFF, fmt, args);
}

int
sprintf (char *buf, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  int len = vsprintf (buf, fmt, args);
  va_end (args);
  return len;
}

// fat16 has no rename, the file is copied and the original removed
int
rename (const char *from, const char *to)
{
  FILE *in = fopen (from, "rb");
  if (!in)
    {
      return -1;
    }
  FILE *out = fopen (to, "wb");
  if (!out)
    {
      fclose (in);
      return -1;
    }
  char buf[512];
  size_t got;
  int res = 0;
  while ((got = fread (buf, 1, sizeof (buf), in)) > 0)
    {
      if (fwrite (buf, 1, got, out) != got)
        {
          res = -1;
          break;
        }
    }
  fclose (in);
  fclose (out);
  return res == 0 ? remove (from) : res;
}

void
perror (const char *what)
{
  if (what && *what)
    {
      fprintf (stderr, "%s: ", what);
    }
  fprintf (stderr, "%s\n", strerror (errno));
}
