// an example for tcc on moos: tcc /src/hello.c -o hello.elf
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
  long long big = 1234567890123LL;
  printf ("hello from tcc, %d args\n", argc);
  printf ("%lld / 7 = %lld, %.2f\n", big, big / 7, 3.25 * 2);
  return argc > 1 ? atoi (argv[1]) : 0;
}
