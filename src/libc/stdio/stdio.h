#ifndef STDIO_H
#define STDIO_H

#include <stdint.h>

// custom printf implementation. currently only accepts %d for integers, %s for
// strings, %p for pointers and custom %C for VGA colours. TODO: add rest of the
// printf indicators.
void printf(const char *str, ...);

// tries to open given path, if it is a file that system can manage, calls
// appropriate  file systems open function
int32_t fopen(const char *file_name, const char *mode);

#endif // !STDIO_H
