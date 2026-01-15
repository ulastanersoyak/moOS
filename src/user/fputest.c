#include "lib/moos.h"

// two copies run at the same time with different fpu control words (double
// and single precision) and compute pi with the leibniz series. the kernel
// has to keep the fpu registers of each task apart, otherwise one copy sees
// the control word or the partial sums of the other

#define TERMS 400000
#define CHECK_EVERY 20000

static uint16_t
get_control_word (void)
{
  uint16_t cw;
  __asm__ volatile ("fnstcw %0" : "=m"(cw));
  return cw;
}

static void
set_control_word (uint16_t cw)
{
  __asm__ volatile ("fldcw %0" : : "m"(cw));
}

static int
child (const char *name, uint16_t cw)
{
  set_control_word (cw);
  double sum = 0;
  for (int i = 0; i < TERMS; i++)
    {
      sum += (i & 1 ? -4.0 : 4.0) / (2 * i + 1);
      if (i % CHECK_EVERY == 0)
        {
          if (get_control_word () != cw)
            {
              printf ("fputest %s: control word changed to %x\n", name,
                      get_control_word ());
              return 1;
            }
          // let the other copy run in between
          yield ();
        }
    }
  int micro = (int)(sum * 1000000);
  printf ("fputest %s: pi = %d.%06d\n", name, micro / 1000000,
          micro % 1000000);
  return 0;
}

int
main (int argc, char **argv)
{
  // precision control in bits 8-9: 10 double, 00 single. all exceptions
  // stay masked
  if (argc == 2)
    {
      return child (argv[1], argv[1][0] == 'd' ? 0x027F : 0x007F);
    }
  char *double_argv[] = { "fputest", "double", 0 };
  char *single_argv[] = { "fputest", "single", 0 };
  int pids[2] = { spawn ("fputest", double_argv, 0),
                  spawn ("fputest", single_argv, 0) };
  int failed = 0;
  for (int i = 0; i < 2; i++)
    {
      failed |= pids[i] < 0 || wait (pids[i]) != 0;
    }
  printf ("fputest: %s\n", failed ? "failed" : "ok");
  return failed;
}
