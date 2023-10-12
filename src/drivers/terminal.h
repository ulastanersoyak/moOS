#ifndef SCREEN_H
#define SCREEN_H
#include <stddef.h>

void terminal_clean(void);
void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char *data, size_t size);
void terminal_writestring(const char *data);

#endif // !SCREEN_H
