#ifndef STDIO_H
#define STDIO_H

#include "../../file_system/file.h"
#include <stdint.h>

// custom printf implementation. currently only accepts %d for integers, %s for
// strings, %p for pointers and custom %C for VGA colours. TODO: add rest of
// the printf indicators.
void printf (const char *str, ...);

int fopen (const char *filename, const char *mode_str);
int fseek (int fd, int offset, FILE_SEEK_MODE whence);
int fread (void *ptr, uint32_t size, uint32_t nmemb, int fd);
int fstat (int fd, struct file_stat *stat);
int fclose (int fd);

#endif // !STDIO_H
