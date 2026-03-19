#ifndef VGA_GFX_H
#define VGA_GFX_H

#include <stdint.h>

#define VGA_MODE_TEXT 0

// vga mode 13h: 320x200 pixels, one byte per pixel indexing a palette of
// 256 colours, the frame is a plain array at 0xA0000
#define VGA_MODE_256 1
#define VGA_GFX_WIDTH 320
#define VGA_GFX_HEIGHT 200

// vga mode 11h: 640x480 pixels in black and white, 8 pixels per byte with
// the leftmost one in the highest bit. a set bit is black
#define VGA_MODE_MONO 2
#define VGA_MONO_WIDTH 640
#define VGA_MONO_HEIGHT 480

// switches to one of the graphics modes. the text screen, the font and the
// palette of the text mode are saved and printing goes on in the background
void vga_gfx_enter (uint8_t mode);

// switches back to the 80x25 text mode and restores what enter saved
void vga_gfx_leave (void);

// the VGA_MODE_ the screen is in
uint8_t vga_gfx_active (void);

// bytes in a frame of the current graphics mode, 0 in the text mode
uint32_t vga_gfx_frame_size (void);

// sets count palette entries from first on, rgb holds 3 bytes (0-255) per
// colour
void vga_gfx_set_palette (const uint8_t *rgb, uint32_t first, uint32_t count);

// copies a frame of vga_gfx_frame_size bytes to the screen
void vga_gfx_blit (const uint8_t *pixels);

#endif // !VGA_GFX_H
