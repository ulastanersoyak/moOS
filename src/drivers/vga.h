#ifndef VGA_H
#define VGA_H

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

static inline uint8_t vga_entry_colour(enum vga_colour fg, enum vga_colour bg) {
  return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t colour) {
  return (uint16_t)uc | (uint16_t)colour << 8;
}

#endif // !VGA_H
