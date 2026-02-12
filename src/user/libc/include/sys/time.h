#ifndef _SYS_TIME_H
#define _SYS_TIME_H

#include <sys/types.h>

struct timeval
{
  time_t tv_sec;
  long tv_usec;
};

// time since boot, there is no wall clock time in seconds
int gettimeofday (struct timeval *tv, void *tz);

#endif
