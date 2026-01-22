#ifndef SCREEN_H
#define SCREEN_H
#include <stddef.h>
#include <stdint.h>

void terminal_clean (void);
void terminal_init (uint8_t verbose);
void terminal_putchar (char c);
void terminal_write (const char *data, size_t size);
void terminal_writestring (const char *data);
void terminal_writeint (int32_t num);
void terminal_writehex (uint32_t num);
void terminal_writeuint (uint32_t num);
void terminal_setcolour (uint8_t colour);
// cursor position, used by the shell to redraw the line being edited
void terminal_get_cursor (size_t *row, size_t *column);
void terminal_set_cursor (size_t row, size_t column);
// number of times the screen scrolled up since boot
uint32_t terminal_scroll_count (void);
// hide copies the text screen into memory and keeps printing there while a
// graphics mode uses the vga memory, show puts it back
void terminal_hide (void);
void terminal_show (void);
void init_OK (void);
void init_ER (void);
#endif // !SCREEN_H
