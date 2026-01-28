#ifndef VGA_GFX_H
#define VGA_GFX_H

#include <stdint.h>

// vga mode 13h: 320x200 pixels, one byte per pixel indexing a palette of
// 256 colours, the frame is a plain array at 0xA0000
#define VGA_GFX_WIDTH 320
#define VGA_GFX_HEIGHT 200

// switches to mode 13h. the text screen, the font and the palette of the
// text mode are saved and printing goes on in the background
void vga_gfx_enter (void);

// switches back to the 80x25 text mode and restores what enter saved
void vga_gfx_leave (void);

uint8_t vga_gfx_active (void);

// sets count palette entries from first on, rgb holds 3 bytes (0-255) per
// colour
void vga_gfx_set_palette (const uint8_t *rgb, uint32_t first, uint32_t count);

// copies a frame of VGA_GFX_WIDTH * VGA_GFX_HEIGHT pixels to the screen
void vga_gfx_blit (const uint8_t *pixels);

#endif // !VGA_GFX_H
