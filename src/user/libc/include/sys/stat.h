#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <sys/types.h>

#define S_IFMT 0170000
#define S_IFDIR 0040000
#define S_IFREG 0100000
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)

struct stat
{
  mode_t st_mode;
  off_t st_size;
};

int stat (const char *path, struct stat *out);
// fat16 has no permissions, mode is ignored
int mkdir (const char *path, mode_t mode);

#endif
