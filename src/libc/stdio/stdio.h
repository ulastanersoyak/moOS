#ifndef STDIO_H
#define STDIO_H

#include "../../file_system/file.h"
#include <stdint.h>

// custom printf implementation. accepts %d and %u for integers (%d prints
// digit groups as 1_000), %x for hex, %c for chars, %s for strings, %% and
// custom %C for VGA colours. TODO: add rest of the printf indicators.
void printf (const char *str, ...);

int fopen (const char *filename, const char *mode_str);
int fseek (int fd, int offset, FILE_SEEK_MODE whence);
int fread (void *ptr, uint32_t size, uint32_t nmemb, int fd);
int fstat (int fd, struct file_stat *stat);
int fclose (int fd);

// lists the directory at path (e.g. 0:/ or 0:/dir). returns number of entries
// written to out or a negative error
int listdir (const char *path, struct file_dirent *out, int max);

// writes nmemb members of size bytes. files must be opened with "w" (create
// or truncate) or "a" (create or append). returns members written
int fwrite (const void *ptr, uint32_t size, uint32_t nmemb, int fd);

// removes a file or an empty directory
int remove (const char *path);

int mkdir (const char *path);

#endif // !STDIO_H
