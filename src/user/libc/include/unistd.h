#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/types.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1

ssize_t read (int fd, void *buf, size_t len);
ssize_t write (int fd, const void *buf, size_t len);
int close (int fd);
off_t lseek (int fd, off_t offset, int whence);
int access (const char *path, int mode);
int unlink (const char *path);
int rmdir (const char *path);
int isatty (int fd);
int usleep (unsigned int usec);
char *getcwd (char *buf, size_t len);
// moos cant replace a program with another one: the program runs as a
// child and this one exits with its exit code once it is done
int execvp (const char *file, char *const argv[]);

#endif
