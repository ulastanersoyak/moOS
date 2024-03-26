#ifndef SCREEN_H
#define SCREEN_H
#include <stddef.h>
#include <stdint.h>

void terminal_clean(void);
void terminal_init(uint8_t verbose);
void terminal_putchar(char c);
void terminal_write(const char *data, size_t size);
void terminal_writestring(const char *data);
void terminal_writeint(int32_t num);
void terminal_writeaddr(void *addr);
void terminal_setcolour(uint8_t colour);
void init_OK(void);
void init_ER(void);
#endif // !SCREEN_H
