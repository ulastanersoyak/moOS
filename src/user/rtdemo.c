#include "lib/moos.h"

// deliberately not a multiple of the 50 ms round robin time slice, otherwise
// wakeups line up with slice boundaries and hide the scheduling delay
#define PERIOD_MS 70
#define PERIODS 20

// periodic task, like a control loop that must run every PERIOD_MS. measures
// how late each activation is. run it next to cpu hogs (spin &) under both
// scheduling policies: round robin makes it wait for every hog's time slice,
// the real time policy runs it as soon as it wakes if its priority is higher
int
main (void)
{
  print ("rtdemo pid ");
  print_int (getpid ());
  print (": waking every ");
  print_int (PERIOD_MS);
  print (" ms\n");

  uint32_t next = uptime ();
  uint32_t max_late = 0;
  uint32_t total_late = 0;
  for (int i = 0; i < PERIODS; i++)
    {
      // absolute deadlines dont drift when an activation runs late
      next += PERIOD_MS;
      sleep_until (next);
      int32_t late = (int32_t)(uptime () - next);
      if (late < 0)
        {
          late = 0;
        }
      total_late += late;
      if ((uint32_t)late > max_late)
        {
          max_late = late;
        }
      putchar ('.');
    }
  print ("\nrtdemo: max lateness ");
  print_int (max_late);
  print (" ms, average ");
  print_int (total_late / PERIODS);
  print (" ms\n");
  return 0;
}
