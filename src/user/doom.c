#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "doomgeneric.h"
#include "doomkeys.h"
#include "i_video.h"

// doom on moos: the doomgeneric platform functions on top of the moos
// graphics, key event and timer system calls. doom renders 320x200 with a
// 256 colour palette (CMAP256), which is vga mode 13h as it is

// moos system calls, see src/user/lib/moos.h. that header cant be included
// next to the c library ones
int gfx_mode (int on);
int gfx_palette (const uint8_t *rgb, int first, int count);
int gfx_blit (const uint8_t *pixels);
int key_event (int flush);
void sleep (uint32_t ms);
uint32_t uptime (void);
int chdir (const char *path);

#define MOOS_KEY_UP 0x80
#define MOOS_KEY_DOWN 0x81
#define MOOS_KEY_LEFT 0x82
#define MOOS_KEY_RIGHT 0x83
#define MOOS_KEY_HOME 0x84
#define MOOS_KEY_END 0x85
#define MOOS_KEY_DELETE 0x86
#define MOOS_KEY_CTRL 0x87
#define MOOS_KEY_SHIFT 0x88
#define MOOS_KEY_ALT 0x89
#define MOOS_KEY_F1 0x8A
#define MOOS_KEY_PAGE_UP 0x96
#define MOOS_KEY_PAGE_DOWN 0x97
#define MOOS_KEY_INSERT 0x98
#define MOOS_KEY_RELEASED 0x100

// where the makefile puts the wad on the disk image
#define DOOM_DIR "0:/doom"
#define DOOM_WAD "0:/doom/doom1.wad"

// set by i_video.c when doom changes its palette
extern boolean palette_changed;
extern struct color colors[256];

static const unsigned char function_keys[] = {
  KEY_F1, KEY_F2, KEY_F3, KEY_F4,  KEY_F5,  KEY_F6,
  KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
};

// doom key code of a moos key event, 0 for keys doom doesnt know
static unsigned char
doom_key (int key)
{
  switch (key)
    {
    case MOOS_KEY_UP:
      return KEY_UPARROW;
    case MOOS_KEY_DOWN:
      return KEY_DOWNARROW;
    case MOOS_KEY_LEFT:
      return KEY_LEFTARROW;
    case MOOS_KEY_RIGHT:
      return KEY_RIGHTARROW;
    case MOOS_KEY_HOME:
      return KEY_HOME;
    case MOOS_KEY_END:
      return KEY_END;
    case MOOS_KEY_DELETE:
      return KEY_DEL;
    case MOOS_KEY_PAGE_UP:
      return KEY_PGUP;
    case MOOS_KEY_PAGE_DOWN:
      return KEY_PGDN;
    case MOOS_KEY_INSERT:
      return KEY_INS;
    case MOOS_KEY_CTRL:
      return KEY_FIRE;
    case MOOS_KEY_SHIFT:
      return KEY_RSHIFT;
    case MOOS_KEY_ALT:
      return KEY_RALT;
    case ' ':
      return KEY_USE;
    case '\n':
      return KEY_ENTER;
    case '\b':
      return KEY_BACKSPACE;
    case 27:
      return KEY_ESCAPE;
    case '\t':
      return KEY_TAB;
    }
  if (key >= MOOS_KEY_F1 && key < MOOS_KEY_F1 + 12)
    {
      return function_keys[key - MOOS_KEY_F1];
    }
  // letters, digits and punctuation are their own (lowercase) chars
  return key < 0x80 ? key : 0;
}

void
DG_Init (void)
{
  gfx_mode (1);
  // drop the keys that started doom
  key_event (1);
}

void
DG_DrawFrame (void)
{
  if (palette_changed)
    {
      uint8_t rgb[256 * 3];
      for (int i = 0; i < 256; i++)
        {
          rgb[i * 3] = colors[i].r;
          rgb[i * 3 + 1] = colors[i].g;
          rgb[i * 3 + 2] = colors[i].b;
        }
      gfx_palette (rgb, 0, 256);
      palette_changed = false;
    }
  gfx_blit ((const uint8_t *)DG_ScreenBuffer);
}

void
DG_SleepMs (uint32_t ms)
{
  sleep (ms);
}

uint32_t
DG_GetTicksMs (void)
{
  return uptime ();
}

int
DG_GetKey (int *pressed, unsigned char *key)
{
  int event;
  while ((event = key_event (0)) >= 0)
    {
      unsigned char doom = doom_key (event & ~MOOS_KEY_RELEASED);
      if (doom)
        {
          *pressed = !(event & MOOS_KEY_RELEASED);
          *key = doom;
          return 1;
        }
    }
  return 0;
}

void
DG_SetWindowTitle (const char *title)
{
}

int
main (int argc, char **argv)
{
  // the config file and savegames go next to the wad
  chdir (DOOM_DIR);
  // play the shareware wad from the disk image
  char *args[32] = { argv[0], "-iwad", DOOM_WAD };
  int count = 3;
  for (int i = 1; i < argc && count < 31; i++)
    {
      args[count++] = argv[i];
    }
  args[count] = 0;
  doomgeneric_Create (count, args);
  while (1)
    {
      doomgeneric_Tick ();
    }
}
