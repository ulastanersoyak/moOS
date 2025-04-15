#include "rtc.h"
#include "../../kernel/io/io.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x02
#define RTC_HOURS 0x04
#define RTC_DAY 0x07
#define RTC_MONTH 0x08
#define RTC_YEAR 0x09
#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B

#define STATUS_A_UPDATING 0x80
#define STATUS_B_24_HOUR 0x02
#define STATUS_B_BINARY 0x04
#define HOUR_PM 0x80

static uint8_t
cmos_read (uint8_t reg)
{
  // bit 7 of the address port would disable nmi, keep it clear
  outb (CMOS_ADDRESS, reg & 0x7F);
  return insb (CMOS_DATA);
}

static void
rtc_read_raw (struct rtc_time *time)
{
  // the clock is only consistent while no update is in progress
  while (cmos_read (RTC_STATUS_A) & STATUS_A_UPDATING)
    {
    }
  time->second = cmos_read (RTC_SECONDS);
  time->minute = cmos_read (RTC_MINUTES);
  time->hour = cmos_read (RTC_HOURS);
  time->day = cmos_read (RTC_DAY);
  time->month = cmos_read (RTC_MONTH);
  time->year = cmos_read (RTC_YEAR);
}

static uint8_t
bcd_to_binary (uint8_t bcd)
{
  return (bcd & 0x0F) + (bcd >> 4) * 10;
}

void
rtc_now (struct rtc_time *time)
{
  // read until two reads agree, an update may start between the registers
  struct rtc_time last;
  rtc_read_raw (time);
  do
    {
      last = *time;
      rtc_read_raw (time);
    }
  while (last.second != time->second || last.minute != time->minute
         || last.hour != time->hour || last.day != time->day
         || last.month != time->month || last.year != time->year);

  uint8_t status = cmos_read (RTC_STATUS_B);
  uint8_t pm = time->hour & HOUR_PM;
  time->hour &= ~HOUR_PM;
  if (!(status & STATUS_B_BINARY))
    {
      time->second = bcd_to_binary (time->second);
      time->minute = bcd_to_binary (time->minute);
      time->hour = bcd_to_binary (time->hour);
      time->day = bcd_to_binary (time->day);
      time->month = bcd_to_binary (time->month);
      time->year = bcd_to_binary (time->year);
    }
  if (!(status & STATUS_B_24_HOUR))
    {
      // 12 hour clock: 12 am is 0, 1 pm is 13
      time->hour %= 12;
      if (pm)
        {
          time->hour += 12;
        }
    }
  // the year register only holds two digits
  time->year += 2000;
}

uint16_t
rtc_fat_date (void)
{
  struct rtc_time now;
  rtc_now (&now);
  return ((now.year - 1980) << 9) | (now.month << 5) | now.day;
}

uint16_t
rtc_fat_time (void)
{
  struct rtc_time now;
  rtc_now (&now);
  return (now.hour << 11) | (now.minute << 5) | (now.second / 2);
}
