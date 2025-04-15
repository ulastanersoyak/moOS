#ifndef RTC_H
#define RTC_H

#include <stdint.h>

struct rtc_time
{
  uint16_t year; // e.g. 2026
  uint8_t month; // 1-12
  uint8_t day;   // 1-31
  uint8_t hour;  // 0-23
  uint8_t minute;
  uint8_t second;
};

// reads the current date and time from the cmos real time clock
void rtc_now (struct rtc_time *time);

// current time in fat directory entry format
uint16_t rtc_fat_date (void); // bits 15-9 year - 1980, 8-5 month, 4-0 day
uint16_t rtc_fat_time (void); // bits 15-11 hour, 10-5 minute, 4-0 second / 2

#endif // !RTC_H
