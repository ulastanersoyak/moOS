#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../libc/string/string.h"
#include "terminal.h"
#include "vga.h"

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_colour;
static uint16_t *terminal_buffer;

static uint16_t *const VGA_MEMORY = (uint16_t *)0xb8000;
static const size_t VGA_HEIGHT = 25;
static const size_t VGA_WIDTH = 80;

// internal print char function
void terminal_putentryat(const unsigned char c, uint8_t colour, size_t x,
                         size_t y) {
  const size_t idx = (y * VGA_WIDTH) + x;
  terminal_buffer[idx] = vga_entry(c, colour);
}

void terminal_clean(void) {
  terminal_row = 0;
  terminal_column = 0;
  terminal_colour = vga_entry_colour(light_grey, black);
  terminal_buffer = VGA_MEMORY;
  for (size_t y = 0; y < VGA_HEIGHT; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t idx = (y * VGA_WIDTH) + x;
      terminal_buffer[idx] = vga_entry(' ', terminal_colour);
    }
  }
}

void terminal_initialize(void) { terminal_clean(); }

void terminal_putchar(char c) {
  unsigned char uc = c;
  terminal_putentryat(c, terminal_colour, terminal_column, terminal_row);
  if (++terminal_column == VGA_WIDTH) {
    terminal_column = 0;
  }
  if (++terminal_row == VGA_HEIGHT) {
    terminal_column = 0;
  }
}

void terminal_write(const char *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    terminal_putchar(data[i]);
  }
}

void terminal_writestring(const char *data) {
  terminal_write(data, strlen(data));
}
