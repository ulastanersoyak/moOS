#ifndef _FCNTL_H
#define _FCNTL_H

#include <sys/types.h>

// moos opens a file for reading, for writing from the start (the file is
// created or made empty, with or without O_CREAT and O_TRUNC) or with
// O_APPEND for writing at the end. O_RDWR isnt supported
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0x40
#define O_TRUNC 0x200
#define O_APPEND 0x400

int open (const char *path, int flags, ...);

#endif
