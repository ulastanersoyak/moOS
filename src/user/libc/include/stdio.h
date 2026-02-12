#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>
#include <stddef.h>

// standard c i/o on top of the moos system calls. files are unbuffered,
// every fread and fwrite is a system call

#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define BUFSIZ 512
#define FILENAME_MAX 108

typedef struct _FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

// modes "r", "w" and "a", with an optional b. "+" opens like without it
FILE *fopen (const char *path, const char *mode);
int fclose (FILE *file);
size_t fread (void *buf, size_t size, size_t count, FILE *file);
size_t fwrite (const void *buf, size_t size, size_t count, FILE *file);
int fseek (FILE *file, long offset, int whence);
long ftell (FILE *file);
void rewind (FILE *file);
int feof (FILE *file);
int ferror (FILE *file);
void clearerr (FILE *file);
int fflush (FILE *file);

int fgetc (FILE *file);
int getc (FILE *file);
int getchar (void);
int ungetc (int c, FILE *file);
char *fgets (char *buf, int size, FILE *file);
int fputc (int c, FILE *file);
int putc (int c, FILE *file);
int putchar (int c);
int fputs (const char *str, FILE *file);
int puts (const char *str);

int printf (const char *fmt, ...);
int fprintf (FILE *file, const char *fmt, ...);
int sprintf (char *buf, const char *fmt, ...);
int snprintf (char *buf, size_t size, const char *fmt, ...);
int vprintf (const char *fmt, va_list args);
int vfprintf (FILE *file, const char *fmt, va_list args);
int vsprintf (char *buf, const char *fmt, va_list args);
int vsnprintf (char *buf, size_t size, const char *fmt, va_list args);

int sscanf (const char *str, const char *fmt, ...);
int fscanf (FILE *file, const char *fmt, ...);
int vsscanf (const char *str, const char *fmt, va_list args);
int vfscanf (FILE *file, const char *fmt, va_list args);

int remove (const char *path);
int rename (const char *from, const char *to);
void perror (const char *what);

#endif
