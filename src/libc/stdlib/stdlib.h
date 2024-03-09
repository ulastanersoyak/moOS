#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>
#include <stdint.h>

void *malloc(size_t size);
void *calloc(size_t size);
void free(void *ptr);


// ascii to integer
uint32_t atoi(const char *buffer);

//TODO: DECIMAL, OCTAL, HEX handle negative numbers
// integer to ascii 
char *itoa(int32_t integer, char *buffer,int8_t base);
#endif // !STDLIB_H
