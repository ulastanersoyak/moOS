#ifndef STDIO_H
#define STDIO_H

#include <stdint.h>


void printf(const char *str, ...);

int32_t fopen(const char* file_name, const char* mode);

#endif// !STDIO_H
