#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

void *malloc (size_t size);
void *calloc (size_t count, size_t size);
void *realloc (void *ptr, size_t size);
void free (void *ptr);

__attribute__ ((noreturn)) void exit (int code);
__attribute__ ((noreturn)) void abort (void);

int atoi (const char *str);
long atol (const char *str);
double atof (const char *str);
long strtol (const char *str, char **end, int base);
unsigned long strtoul (const char *str, char **end, int base);
long long strtoll (const char *str, char **end, int base);
double strtod (const char *str, char **end);

int abs (int num);
long labs (long num);

int rand (void);
void srand (unsigned int seed);

void qsort (void *base, size_t count, size_t size,
            int (*compare) (const void *, const void *));
void *bsearch (const void *key, const void *base, size_t count, size_t size,
               int (*compare) (const void *, const void *));

// there is no environment and no command interpreter: getenv returns 0,
// system returns -1
char *getenv (const char *name);
int system (const char *command);

#endif
