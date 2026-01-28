#include "lib/moos.h"

// draws moving rings and a checkered pattern in vga mode 13h for a few
// seconds, then goes back to the text screen

#define RUN_MS 3000

static uint8_t frame[GFX_WIDTH * GFX_HEIGHT];
static uint8_t palette[256 * 3];

// a smooth rainbow: each channel ramps up and down at a different offset
static uint8_t
ramp (int i)
{
  i &= 255;
  return i < 128 ? i * 2 : (255 - i) * 2;
}

int
main (void)
{
  for (int i = 0; i < 256; i++)
    {
      palette[i * 3] = ramp (i);
      palette[i * 3 + 1] = ramp (i + 85);
      palette[i * 3 + 2] = ramp (i + 170);
    }
  if (gfx_mode (1) < 0)
    {
      eprint ("gfxdemo: the screen is in use\n");
      return 1;
    }
  gfx_palette (palette, 0, 256);
  uint32_t start = uptime ();
  uint32_t frames = 0;
  for (uint32_t t = 0; uptime () - start < RUN_MS; t += 3)
    {
      for (int y = 0; y < GFX_HEIGHT; y++)
        {
          int dy = y - GFX_HEIGHT / 2;
          for (int x = 0; x < GFX_WIDTH; x++)
            {
              int dx = x - GFX_WIDTH / 2;
              // rings around the centre plus a checkered overlay
              uint32_t rings = (uint32_t)(dx * dx + dy * dy) >> 6;
              uint32_t checks = ((x >> 4) ^ (y >> 4)) & 1 ? 32 : 0;
              frame[y * GFX_WIDTH + x] = rings - t + checks;
            }
        }
      gfx_blit (frame);
      frames++;
    }
  gfx_mode (0);
  printf ("gfxdemo: %u frames in %u ms\n", frames, uptime () - start);
  return 0;
}
