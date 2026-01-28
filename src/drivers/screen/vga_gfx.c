#include "vga_gfx.h"
#include "../../kernel/io/io.h"
#include "../../libc/string/string.h"
#include "terminal.h"

#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5
#define VGA_DAC_READ_INDEX 0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA 0x3C9
#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA 0x3CF
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_INSTAT_READ 0x3DA

#define VGA_NUM_SEQ_REGS 5
#define VGA_NUM_CRTC_REGS 25
#define VGA_NUM_GC_REGS 9
#define VGA_NUM_AC_REGS 21

#define VGA_FRAMEBUFFER ((uint8_t *)0xA0000)
// the text mode font lives in plane 2, 32 bytes per char
#define FONT_SIZE (256 * 32)

// register values of both modes: misc, sequencer, crtc, graphics
// controller, attribute controller
static const uint8_t mode_13h[] = {
  0x63,
  0x03, 0x01, 0x0F, 0x00, 0x0E,
  0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0x41, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
  0xFF,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
  0x0C, 0x0D, 0x0E, 0x0F, 0x41, 0x00, 0x0F, 0x00, 0x00,
};

static const uint8_t mode_text_80x25[] = {
  0x67,
  0x03, 0x00, 0x03, 0x00, 0x02,
  0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F, 0x00, 0x4F, 0x0D, 0x0E,
  0x00, 0x00, 0x00, 0x50, 0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
  0xFF,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00, 0xFF,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
  0x3C, 0x3D, 0x3E, 0x3F, 0x0C, 0x00, 0x0F, 0x08, 0x00,
};

static uint8_t active = 0;
static uint8_t saved_font[FONT_SIZE];
static uint8_t saved_palette[256 * 3];

static void
vga_write_regs (const uint8_t *regs)
{
  outb (VGA_MISC_WRITE, *regs++);
  for (uint8_t i = 0; i < VGA_NUM_SEQ_REGS; i++)
    {
      outb (VGA_SEQ_INDEX, i);
      outb (VGA_SEQ_DATA, *regs++);
    }
  // crtc registers 0-7 are write protected by bit 7 of register 0x11
  outb (VGA_CRTC_INDEX, 0x03);
  outb (VGA_CRTC_DATA, insb (VGA_CRTC_DATA) | 0x80);
  outb (VGA_CRTC_INDEX, 0x11);
  outb (VGA_CRTC_DATA, insb (VGA_CRTC_DATA) & ~0x80);
  for (uint8_t i = 0; i < VGA_NUM_CRTC_REGS; i++)
    {
      uint8_t value = regs[i];
      // keep them unlocked while writing
      if (i == 0x03)
        {
          value |= 0x80;
        }
      else if (i == 0x11)
        {
          value &= ~0x80;
        }
      outb (VGA_CRTC_INDEX, i);
      outb (VGA_CRTC_DATA, value);
    }
  regs += VGA_NUM_CRTC_REGS;
  for (uint8_t i = 0; i < VGA_NUM_GC_REGS; i++)
    {
      outb (VGA_GC_INDEX, i);
      outb (VGA_GC_DATA, *regs++);
    }
  // reading the input status resets the attribute controller to its index
  // state, then index and value alternate on the same port
  for (uint8_t i = 0; i < VGA_NUM_AC_REGS; i++)
    {
      insb (VGA_INSTAT_READ);
      outb (VGA_AC_INDEX, i);
      outb (VGA_AC_WRITE, *regs++);
    }
  // turn the display back on
  insb (VGA_INSTAT_READ);
  outb (VGA_AC_INDEX, 0x20);
}

static uint8_t
vga_read (uint16_t index_port, uint8_t index)
{
  outb (index_port, index);
  return insb (index_port + 1);
}

// copies the font in plane 2 from or to buf. only works in the text mode,
// the registers it changes are restored
static void
vga_font_copy (uint8_t *buf, uint8_t save)
{
  uint8_t seq2 = vga_read (VGA_SEQ_INDEX, 2);
  uint8_t seq4 = vga_read (VGA_SEQ_INDEX, 4);
  uint8_t gc4 = vga_read (VGA_GC_INDEX, 4);
  uint8_t gc5 = vga_read (VGA_GC_INDEX, 5);
  uint8_t gc6 = vga_read (VGA_GC_INDEX, 6);
  // flat addressing of plane 2 only, mapped at 0xA0000
  outb (VGA_SEQ_INDEX, 2);
  outb (VGA_SEQ_DATA, 1 << 2);
  outb (VGA_SEQ_INDEX, 4);
  outb (VGA_SEQ_DATA, seq4 | 0x04);
  outb (VGA_GC_INDEX, 4);
  outb (VGA_GC_DATA, 2);
  outb (VGA_GC_INDEX, 5);
  outb (VGA_GC_DATA, gc5 & ~0x10);
  outb (VGA_GC_INDEX, 6);
  outb (VGA_GC_DATA, (gc6 & ~0x0E) | 0x04);
  if (save)
    {
      memcpy (buf, VGA_FRAMEBUFFER, FONT_SIZE);
    }
  else
    {
      memcpy (VGA_FRAMEBUFFER, buf, FONT_SIZE);
    }
  outb (VGA_SEQ_INDEX, 2);
  outb (VGA_SEQ_DATA, seq2);
  outb (VGA_SEQ_INDEX, 4);
  outb (VGA_SEQ_DATA, seq4);
  outb (VGA_GC_INDEX, 4);
  outb (VGA_GC_DATA, gc4);
  outb (VGA_GC_INDEX, 5);
  outb (VGA_GC_DATA, gc5);
  outb (VGA_GC_INDEX, 6);
  outb (VGA_GC_DATA, gc6);
}

// the dac holds 6 bits per colour channel
static void
vga_palette_copy (uint8_t *buf, uint8_t save)
{
  if (save)
    {
      outb (VGA_DAC_READ_INDEX, 0);
      for (uint32_t i = 0; i < sizeof (saved_palette); i++)
        {
          buf[i] = insb (VGA_DAC_DATA);
        }
    }
  else
    {
      outb (VGA_DAC_WRITE_INDEX, 0);
      for (uint32_t i = 0; i < sizeof (saved_palette); i++)
        {
          outb (VGA_DAC_DATA, buf[i]);
        }
    }
}

void
vga_gfx_enter (void)
{
  if (active)
    {
      return;
    }
  terminal_hide ();
  vga_font_copy (saved_font, 1);
  vga_palette_copy (saved_palette, 1);
  vga_write_regs (mode_13h);
  memset (VGA_FRAMEBUFFER, 0, VGA_GFX_WIDTH * VGA_GFX_HEIGHT);
  active = 1;
}

void
vga_gfx_leave (void)
{
  if (!active)
    {
      return;
    }
  vga_write_regs (mode_text_80x25);
  vga_font_copy (saved_font, 0);
  vga_palette_copy (saved_palette, 0);
  active = 0;
  terminal_show ();
}

uint8_t
vga_gfx_active (void)
{
  return active;
}

void
vga_gfx_set_palette (const uint8_t *rgb, uint32_t first, uint32_t count)
{
  outb (VGA_DAC_WRITE_INDEX, first);
  for (uint32_t i = 0; i < count * 3; i++)
    {
      outb (VGA_DAC_DATA, rgb[i] >> 2);
    }
}

void
vga_gfx_blit (const uint8_t *pixels)
{
  memcpy (VGA_FRAMEBUFFER, pixels, VGA_GFX_WIDTH * VGA_GFX_HEIGHT);
}
