#ifndef _FCNTL_H
#define _FCNTL_H

#include <sys/types.h>

// only O_RDONLY and O_WRONLY | O_CREAT | O_TRUNC match what the moos open
// system call does (modes 0 and 1), use fopen for anything else
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0x40
#define O_TRUNC 0x200
#define O_APPEND 0x400

int open (const char *path, int flags, ...);

#endif
