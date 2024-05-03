#ifndef ASCII_H
#define ASCII_H

#include "../../drivers/screen/vga.h"

void cowsay (const char *str, enum vga_colour colour);

void shrigma (void);

void moose (const char *str, enum vga_colour colour);

#endif // !ASCII_H
