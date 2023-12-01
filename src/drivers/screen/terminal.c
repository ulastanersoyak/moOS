#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #include "../libc/string/string.h"
#include "../../libc/string/string.h"
#include "terminal.h"
#include "vga.h"

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_colour;
static uint16_t *terminal_buffer;

// puts empty char on whole screen
void terminal_clean(void) {
  terminal_row = 0;
  terminal_column = 0;
  for (size_t y = 0; y < VGA_HEIGHT; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t idx = (y * VGA_WIDTH) + x;
      terminal_buffer[idx] = vga_entry(' ', terminal_colour);
    }
  }
}

// initializes terminal attribiutes and clens the screen
void terminal_initialize(void) {
  terminal_colour = vga_entry_colour(white, black);
  terminal_buffer = VGA_MEMORY;
  terminal_clean();
  terminal_writestring("terminal initializing");
  terminal_setcolour(green);
  terminal_writestring(" [OK]\n");
  terminal_setcolour(white);
}
void terminal_setcolour(uint8_t colour) { terminal_colour = colour; }

// internal print char function
void terminal_putentryat(const unsigned char c, uint8_t colour, size_t x,
                         size_t y) {
  const size_t idx = (y * VGA_WIDTH) + x;
  terminal_buffer[idx] = vga_entry(c, colour);
}

// wrapper of terminal_putentryat function.
void terminal_putchar(char c) {
  unsigned char uc = c;
  if (uc == '\n') {
    terminal_column = 0;
    terminal_row += 1;
    return;
  }
  terminal_putentryat(uc, terminal_colour, terminal_column, terminal_row);
  if (++terminal_column == VGA_WIDTH) {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT) {
      terminal_row = 0;
    }
  }
}

// wrapper of terminal_putchar function
void terminal_write(const char *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    terminal_putchar(data[i]);
  }
}

// wrapper of terminal_write function.
void terminal_writestring(const char *data) {
  terminal_write(data, strlen(data));
}
