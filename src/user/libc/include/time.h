#ifndef _TIME_H
#define _TIME_H

#include <sys/types.h>

typedef long clock_t;
#define CLOCKS_PER_SEC 1000

struct tm
{
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;  // 0 to 11
  int tm_year; // years since 1900
  int tm_wday;
  int tm_yday;
  int tm_isdst;
};

// seconds since 1970 from the cmos clock, which holds local time
time_t time (time_t *out);
clock_t clock (void);
time_t mktime (struct tm *tm);
struct tm *localtime (const time_t *t);
struct tm *gmtime (const time_t *t);

#endif
