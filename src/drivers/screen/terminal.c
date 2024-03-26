#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../../libc/string/string.h"
#include "../../libc/stdio/stdio.h"
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
void terminal_init(uint8_t verbose) {
  terminal_colour = vga_entry_colour(white, black);
  terminal_buffer = VGA_MEMORY;
  terminal_clean();
  if(verbose){
    printf("terminal init");
    init_OK();
  }
}

void terminal_setcolour(uint8_t colour) { terminal_colour = colour; }

// internal print char function
void terminal_putentryat(const unsigned char c, uint8_t colour, size_t x, size_t y) {
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
  }else if(uc =='\t'){
    terminal_column+=4;
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

// TODO: negative number implementation
void terminal_writeint(int32_t num) {
  char map[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
  size_t idx = 0;
  uint32_t digits[100];
  if(num < 0 ){
    terminal_putchar('-');
    num*=-1;
  }
  if (num == 0) {
    terminal_putchar(map[num]);
    return;
  }
  while (num > 0) {
    uint32_t digit = num % 10;
    digits[idx] = digit;
    idx++;
    num /= 10;
  }
  for (int32_t i = idx - 1; i >= 0; i--) {
    if ((i - 2) % 3 == 0 && i != idx - 1) {
      terminal_putchar('_');
    }
    terminal_putchar(map[digits[i]]);
  }
}

void terminal_writeaddr(void *addr) {
  uint32_t address = (uint32_t)addr;
  char hex_string[9];
  hex_string[0] = '0';
  hex_string[1] = 'x';
  for (int i = 0; i < 8; i++) {
    int32_t nibble = (address >> (28 - 4 * i)) & 0xF;
    hex_string[2 + i] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
  }
  terminal_writestring(hex_string);
}

void init_OK(void){
  terminal_setcolour(green);
  terminal_writestring(" [OK]\n");
  terminal_setcolour(white);
}

void init_ER(void){
  terminal_setcolour(red);
  terminal_writestring(" [ER]\n");
  terminal_setcolour(white);
}
