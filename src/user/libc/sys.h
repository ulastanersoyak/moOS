#ifndef LIBC_SYS_H
#define LIBC_SYS_H

#include <stdint.h>

// moos system calls the c library is built on, see src/user/lib/moos.h.
// declared here because some of their names mean something else in
// standard c (open takes flags, sleep takes seconds there). open, time and
// getcwd are replaced by the standard ones, moos_<name> is the system call

#define MOOS_READ 0
#define MOOS_WRITE 1
#define MOOS_APPEND 2

struct moos_dirent
{
  char name[64];
  uint32_t size;
  uint8_t is_dir;
  uint16_t date;
  uint16_t time;
};

struct moos_datetime
{
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

int moos_open (const char *path, int mode);
int moos_time (struct moos_datetime *now);
int moos_getcwd (char *buf, uint32_t len);
int seek (int fd, int32_t offset, int whence);
int filesize (int fd);
int spawn (const char *program, char **argv, const int io[2]);
int wait (int pid);
int listdir (const char *path, struct moos_dirent *out, int max);
void sleep (uint32_t ms);
uint32_t uptime (void);

#endif
