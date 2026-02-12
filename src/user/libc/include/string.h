#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

void *memcpy (void *dest, const void *src, size_t len);
void *memmove (void *dest, const void *src, size_t len);
void *memset (void *dest, int c, size_t len);
int memcmp (const void *a, const void *b, size_t len);
void *memchr (const void *ptr, int c, size_t len);

size_t strlen (const char *str);
size_t strnlen (const char *str, size_t max);
int strcmp (const char *a, const char *b);
int strncmp (const char *a, const char *b, size_t n);
char *strcpy (char *dest, const char *src);
char *strncpy (char *dest, const char *src, size_t n);
char *strcat (char *dest, const char *src);
char *strncat (char *dest, const char *src, size_t n);
char *strchr (const char *str, int c);
char *strrchr (const char *str, int c);
char *strstr (const char *haystack, const char *needle);
size_t strspn (const char *str, const char *accept);
size_t strcspn (const char *str, const char *reject);
char *strpbrk (const char *str, const char *accept);
char *strtok (char *str, const char *delim);
char *strdup (const char *str);
char *strndup (const char *str, size_t max);
char *strerror (int err);

#endif
