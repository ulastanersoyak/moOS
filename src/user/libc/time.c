#include "sys.h"
#include <time.h>

// the cmos clock holds local time without a time zone, so local time and
// utc are the same here

static const int month_days[12]
    = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static int
is_leap (int year)
{
  return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

// days from 1970-01-01, month from 0
static long
days_since_epoch (int year, int month, int day)
{
  long days = 0;
  for (int y = 1970; y < year; y++)
    {
      days += is_leap (y) ? 366 : 365;
    }
  for (int m = 0; m < month; m++)
    {
      days += month_days[m] + (m == 1 && is_leap (year));
    }
  return days + day - 1;
}

time_t
time (time_t *out)
{
  struct moos_datetime now;
  time_t t = -1;
  if (moos_time (&now) >= 0)
    {
      t = days_since_epoch (now.year, now.month - 1, now.day) * 86400
          + now.hour * 3600 + now.minute * 60 + now.second;
    }
  if (out)
    {
      *out = t;
    }
  return t;
}

clock_t
clock (void)
{
  return uptime ();
}

time_t
mktime (struct tm *tm)
{
  return days_since_epoch (tm->tm_year + 1900, tm->tm_mon, tm->tm_mday)
             * 86400
         + tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
}

struct tm *
gmtime (const time_t *t)
{
  static struct tm tm;
  long days = *t / 86400;
  long secs = *t % 86400;
  tm.tm_hour = secs / 3600;
  tm.tm_min = secs / 60 % 60;
  tm.tm_sec = secs % 60;
  tm.tm_wday = (days + 4) % 7; // 1970-01-01 was a thursday
  int year = 1970;
  while (days >= (is_leap (year) ? 366 : 365))
    {
      days -= is_leap (year) ? 366 : 365;
      year++;
    }
  tm.tm_year = year - 1900;
  tm.tm_yday = days;
  int month = 0;
  while (days >= month_days[month] + (month == 1 && is_leap (year)))
    {
      days -= month_days[month] + (month == 1 && is_leap (year));
      month++;
    }
  tm.tm_mon = month;
  tm.tm_mday = days + 1;
  tm.tm_isdst = 0;
  return &tm;
}

struct tm *
localtime (const time_t *t)
{
  return gmtime (t);
}
