#ifndef VGA_H
#define VGA_H

#include <stddef.h>
#include <stdint.h>
enum vga_colour {
  black,
  blue,
  green,
  cyan,
  red,
  brown,
  light_grey,
  dark_grey,
  light_blue,
  light_green,
  light_cyan,
  light_red,
  light_magenta,
  light_brown,
  white,
};

static uint16_t *const VGA_MEMORY = (uint16_t *)0xb8000;
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

// set vga colour bit with background and foreground colours
static inline uint8_t vga_entry_colour(enum vga_colour fg, enum vga_colour bg) {
  return fg | bg << 4;
}
// returns 2 byte. first byte contains char data while second byte contains
// colour data. written in reverse because of little endian architecture
static inline uint16_t vga_entry(unsigned char uc, uint8_t colour) {
  return (uint16_t)colour << 8 | (uint16_t)uc;
}

#endif // !VGA_H
