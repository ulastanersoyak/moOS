#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../../kernel/config.h"
#include "../../kernel/io/io.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/string/string.h"
#include "../serial/serial.h"
#include "terminal.h"
#include "vga.h"

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_colour;
static uint16_t *terminal_buffer;
static uint32_t terminal_scrolls = 0;
// the text while a program has the screen in a graphics mode, printing goes
// on here and shows up again when the text mode comes back
// 80x25, VGA_WIDTH and VGA_HEIGHT arent constant expressions in c
static uint16_t hidden_buffer[80 * 25];

// moves the blinking vga cursor to current position
static void
terminal_update_cursor (void)
{
  if (terminal_buffer == hidden_buffer)
    {
      return;
    }
  uint16_t pos = terminal_row * VGA_WIDTH + terminal_column;
  outb (0x3D4, 0x0F);
  outb (0x3D5, pos & 0xFF);
  outb (0x3D4, 0x0E);
  outb (0x3D5, (pos >> 8) & 0xFF);
}

// puts empty char on whole screen
void
terminal_clean (void)
{
  terminal_row = 0;
  terminal_column = 0;
  for (size_t y = 0; y < VGA_HEIGHT; y++)
    {
      for (size_t x = 0; x < VGA_WIDTH; x++)
        {
          const size_t idx = (y * VGA_WIDTH) + x;
          terminal_buffer[idx] = vga_entry (' ', terminal_colour);
        }
    }
  terminal_update_cursor ();
}

// initializes terminal attributes and cleans the screen
void
terminal_init (uint8_t verbose)
{
  terminal_colour = vga_entry_colour (white, black);
  terminal_buffer = VGA_MEMORY;
#if SERIAL_CONSOLE
  serial_init ();
#endif
  terminal_clean ();
  if (verbose)
    {
      printf ("vga driver init");
      init_OK ();
    }
}

void
terminal_setcolour (uint8_t colour)
{
  terminal_colour = colour;
}

// internal print char function
void
terminal_putentryat (const unsigned char c, uint8_t colour, size_t x, size_t y)
{
  const size_t idx = (y * VGA_WIDTH) + x;
  terminal_buffer[idx] = vga_entry (c, colour);
}

// moves every row one line up and clears the last row
static void
terminal_scroll (void)
{
  memmove (terminal_buffer, terminal_buffer + VGA_WIDTH,
           (VGA_HEIGHT - 1) * VGA_WIDTH * sizeof (uint16_t));
  for (size_t x = 0; x < VGA_WIDTH; x++)
    {
      terminal_putentryat (' ', terminal_colour, x, VGA_HEIGHT - 1);
    }
  terminal_row = VGA_HEIGHT - 1;
  terminal_scrolls++;
}

static void
terminal_newline (void)
{
  terminal_column = 0;
  if (++terminal_row == VGA_HEIGHT)
    {
      terminal_scroll ();
    }
}

// wrapper of terminal_putentryat function.
void
terminal_putchar (char c)
{
  unsigned char uc = c;
#if DEBUG_CONSOLE
  // mirror output to qemu's debug console (-debugcon)
  outb (DEBUG_CONSOLE_PORT, uc);
#endif
#if SERIAL_CONSOLE
  serial_putchar (uc);
#endif
  if (uc == '\n')
    {
      terminal_newline ();
      terminal_update_cursor ();
      return;
    }
  else if (uc == '\b')
    {
      // erase previous char, wrapping back to the end of the previous row
      if (terminal_column > 0)
        {
          terminal_column--;
        }
      else if (terminal_row > 0)
        {
          terminal_row--;
          terminal_column = VGA_WIDTH - 1;
        }
      terminal_putentryat (' ', terminal_colour, terminal_column,
                           terminal_row);
      terminal_update_cursor ();
      return;
    }
  else if (uc == '\t')
    {
      terminal_column += 4;
      if (terminal_column >= VGA_WIDTH)
        {
          terminal_newline ();
        }
      terminal_update_cursor ();
      return;
    }
  terminal_putentryat (uc, terminal_colour, terminal_column, terminal_row);
  if (++terminal_column == VGA_WIDTH)
    {
      terminal_newline ();
    }
  terminal_update_cursor ();
}

// wrapper of terminal_putchar function
void
terminal_write (const char *data, size_t size)
{
  for (size_t i = 0; i < size; i++)
    {
      terminal_putchar (data[i]);
    }
}

// wrapper of terminal_write function.
void
terminal_writestring (const char *data)
{
  terminal_write (data, strlen (data));
}

void
terminal_writeint (int32_t num)
{
  char map[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
  size_t idx = 0;
  uint32_t digits[10];
  // use unsigned magnitude so INT32_MIN doesnt overflow on negation
  uint32_t mag = (uint32_t)num;
  if (num < 0)
    {
      terminal_putchar ('-');
      mag = -mag;
    }
  if (mag == 0)
    {
      terminal_putchar (map[0]);
      return;
    }
  while (mag > 0)
    {
      digits[idx] = mag % 10;
      idx++;
      mag /= 10;
    }
  for (int32_t i = idx - 1; i >= 0; i--)
    {
      if ((i - 2) % 3 == 0 && i != (int32_t)idx - 1)
        {
          terminal_putchar ('_');
        }
      terminal_putchar (map[digits[i]]);
    }
}

void
terminal_writeuint (uint32_t num)
{
  char digits[10];
  size_t idx = 0;
  do
    {
      digits[idx++] = '0' + (num % 10);
      num /= 10;
    }
  while (num > 0);
  while (idx > 0)
    {
      terminal_putchar (digits[--idx]);
    }
}

void
terminal_writehex (uint32_t num)
{
  const char *map = "0123456789abcdef";
  terminal_writestring ("0x");
  for (int32_t shift = 28; shift >= 0; shift -= 4)
    {
      terminal_putchar (map[(num >> shift) & 0xF]);
    }
}

void
terminal_get_cursor (size_t *row, size_t *column)
{
  *row = terminal_row;
  *column = terminal_column;
}

void
terminal_set_cursor (size_t row, size_t column)
{
  terminal_row = row < VGA_HEIGHT ? row : VGA_HEIGHT - 1;
  terminal_column = column < VGA_WIDTH ? column : VGA_WIDTH - 1;
  terminal_update_cursor ();
}

uint32_t
terminal_scroll_count (void)
{
  return terminal_scrolls;
}

void
terminal_hide (void)
{
  memcpy (hidden_buffer, VGA_MEMORY, sizeof (hidden_buffer));
  terminal_buffer = hidden_buffer;
}

void
terminal_show (void)
{
  memcpy (VGA_MEMORY, hidden_buffer, sizeof (hidden_buffer));
  terminal_buffer = VGA_MEMORY;
  terminal_update_cursor ();
}

void
init_OK (void)
{
  terminal_setcolour (green);
  terminal_writestring (" [OK]\n");
  terminal_setcolour (white);
}

void
init_ER (void)
{
  terminal_setcolour (red);
  terminal_writestring (" [ER]\n");
  terminal_setcolour (white);
}
