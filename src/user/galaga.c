#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDL_framerate.h"

// xgalaga on moos: the sdl 1.2 calls it makes, on top of the moos graphics,
// key event and timer system calls. the game draws on a 32 bit screen of
// 468x596, as tall as the arcade one, and SDL_Flip shrinks that into the
// middle of vga mode 13h with a 6x6x6 colour cube for a palette

// moos system calls, see src/user/lib/moos.h. that header cant be included
// next to the c library ones
int gfx_mode (int on);
int gfx_palette (const uint8_t *rgb, int first, int count);
int gfx_blit (const uint8_t *pixels);
int key_event (int flush);
void sleep (uint32_t ms);
void sleep_until (uint32_t ms);
uint32_t uptime (void);

#define GFX_256 1
#define GFX_WIDTH 320
#define GFX_HEIGHT 200

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
#define MOOS_KEY_PAGE_UP 0x96
#define MOOS_KEY_PAGE_DOWN 0x97
#define MOOS_KEY_INSERT 0x98
#define MOOS_KEY_RELEASED 0x100

// mode 13h stretches 320x200 over a 4:3 screen, a pixel is 5/6 as wide as
// it is tall. the shrunk screen keeps the game's shape on a 4:3 monitor
#define PIXEL_ASPECT_W 5
#define PIXEL_ASPECT_H 6

static SDL_Surface *screen;
static uint8_t frame[GFX_WIDTH * GFX_HEIGHT];
// where SDL_Flip puts the screen in frame and the screen columns and rows
// each pixel of it takes, out_x[x] to out_x[x + 1]
static int out_left, out_width;
static int out_x[GFX_WIDTH + 1];
static int out_y[GFX_HEIGHT + 1];
static SDLMod mods;

int
atexit (void (*function) (void))
{
  return 0;
}

int
SDL_Init (Uint32 flags)
{
  return 0;
}

int
SDL_InitSubSystem (Uint32 flags)
{
  return 0;
}

void
SDL_Quit (void)
{
}

char *
SDL_GetError (void)
{
  return "the screen is in use";
}

int
SDL_ShowCursor (int toggle)
{
  return SDL_DISABLE;
}

void
SDL_WM_SetCaption (const char *title, const char *icon)
{
}

static int
mask_shift (Uint32 mask)
{
  return mask ? __builtin_ctz (mask) : 0;
}

SDL_Surface *
SDL_CreateRGBSurface (Uint32 flags, int width, int height, int depth,
                      Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
  if (depth != 32 || width <= 0 || height <= 0)
    {
      return 0;
    }
  SDL_Surface *surface = calloc (1, sizeof (SDL_Surface));
  SDL_PixelFormat *format = calloc (1, sizeof (SDL_PixelFormat));
  void *pixels = calloc (width * height, 4);
  if (!surface || !format || !pixels)
    {
      free (surface);
      free (format);
      free (pixels);
      return 0;
    }
  format->BitsPerPixel = 32;
  format->BytesPerPixel = 4;
  format->Rmask = Rmask;
  format->Gmask = Gmask;
  format->Bmask = Bmask;
  format->Amask = Amask;
  format->Rshift = mask_shift (Rmask);
  format->Gshift = mask_shift (Gmask);
  format->Bshift = mask_shift (Bmask);
  format->Ashift = mask_shift (Amask);
  surface->flags = Amask ? flags : flags & ~SDL_SRCALPHA;
  surface->format = format;
  surface->w = width;
  surface->h = height;
  surface->pitch = width * 4;
  surface->pixels = pixels;
  return surface;
}

void
SDL_FreeSurface (SDL_Surface *surface)
{
  if (surface && surface != screen)
    {
      free (surface->pixels);
      free (surface->format);
      free (surface);
    }
}

int
SDL_VideoModeOK (int width, int height, int bpp, Uint32 flags)
{
  return 32;
}

SDL_Surface *
SDL_SetVideoMode (int width, int height, int bpp, Uint32 flags)
{
  // toggling fullscreen asks for the mode it has again
  if (screen)
    {
      return screen;
    }
  screen = SDL_CreateRGBSurface (0, width, height, 32, 0x00FF0000, 0x0000FF00,
                                 0x000000FF, 0);
  if (!screen || gfx_mode (GFX_256) < 0)
    {
      return 0;
    }
  uint8_t rgb[216 * 3];
  for (int i = 0; i < 216; i++)
    {
      rgb[i * 3] = i / 36 * 51;
      rgb[i * 3 + 1] = i / 6 % 6 * 51;
      rgb[i * 3 + 2] = i % 6 * 51;
    }
  gfx_palette (rgb, 0, 216);
  // as wide as the game's shape allows, never wider than the screen
  out_width = width * GFX_HEIGHT * PIXEL_ASPECT_H / PIXEL_ASPECT_W / height;
  if (out_width > GFX_WIDTH)
    {
      out_width = GFX_WIDTH;
    }
  out_left = (GFX_WIDTH - out_width) / 2;
  for (int x = 0; x <= out_width; x++)
    {
      out_x[x] = x * width / out_width;
    }
  for (int y = 0; y <= GFX_HEIGHT; y++)
    {
      out_y[y] = y * height / GFX_HEIGHT;
    }
  // drop the keys that started the game
  key_event (1);
  return screen;
}

SDL_Surface *
SDL_DisplayFormat (SDL_Surface *surface)
{
  return surface;
}

int
SDL_LockSurface (SDL_Surface *surface)
{
  return 0;
}

void
SDL_UnlockSurface (SDL_Surface *surface)
{
}

int
SDL_Flip (SDL_Surface *surface)
{
  // each pixel of the frame covers about 2.5x3 pixels of the screen and
  // takes the brightest of each channel among them, an average would make
  // the stars and the shots fade away
  for (int y = 0; y < GFX_HEIGHT; y++)
    {
      uint8_t *out = frame + y * GFX_WIDTH + out_left;
      for (int x = 0; x < out_width; x++)
        {
          Uint32 r = 0, g = 0, b = 0;
          for (int sy = out_y[y]; sy < out_y[y + 1]; sy++)
            {
              const Uint32 *in = (const Uint32 *)((uint8_t *)screen->pixels
                                                  + sy * screen->pitch);
              for (int sx = out_x[x]; sx < out_x[x + 1]; sx++)
                {
                  Uint32 pixel = in[sx];
                  Uint32 pr = pixel & 0xFF0000, pg = pixel & 0xFF00,
                         pb = pixel & 0xFF;
                  r = pr > r ? pr : r;
                  g = pg > g ? pg : g;
                  b = pb > b ? pb : b;
                }
            }
          // nearest of the 6 levels 0, 51, ... 255 of each channel
          out[x] = ((r >> 16) + 25) / 51 * 36 + ((g >> 8) + 25) / 51 * 6
                   + (b + 25) / 51;
        }
    }
  gfx_blit (frame);
  return 0;
}

Uint32
SDL_MapRGBA (const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b,
             Uint8 a)
{
  return (Uint32)r << format->Rshift | (Uint32)g << format->Gshift
         | (Uint32)b << format->Bshift
         | ((Uint32)a << format->Ashift & format->Amask);
}

Uint32
SDL_MapRGB (const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b)
{
  return SDL_MapRGBA (format, r, g, b, 255);
}

int
SDL_SetColorKey (SDL_Surface *surface, Uint32 flag, Uint32 key)
{
  if (flag)
    {
      surface->flags |= SDL_SRCCOLORKEY;
    }
  else
    {
      surface->flags &= ~SDL_SRCCOLORKEY;
    }
  surface->format->colorkey = key;
  return 0;
}

// clips rect to the surface, 0 if nothing is left of it
static int
clip (SDL_Rect *rect, const SDL_Surface *surface)
{
  int x0 = rect->x < 0 ? 0 : rect->x;
  int y0 = rect->y < 0 ? 0 : rect->y;
  int x1 = rect->x + rect->w;
  int y1 = rect->y + rect->h;
  x1 = x1 > surface->w ? surface->w : x1;
  y1 = y1 > surface->h ? surface->h : y1;
  if (x1 <= x0 || y1 <= y0)
    {
      rect->w = rect->h = 0;
      return 0;
    }
  rect->x = x0;
  rect->y = y0;
  rect->w = x1 - x0;
  rect->h = y1 - y0;
  return 1;
}

int
SDL_FillRect (SDL_Surface *dst, SDL_Rect *rect, Uint32 color)
{
  SDL_Rect area = { 0, 0, dst->w, dst->h };
  if (rect)
    {
      area = *rect;
    }
  if (!clip (&area, dst))
    {
      return 0;
    }
  for (int y = area.y; y < area.y + area.h; y++)
    {
      Uint32 *row = (Uint32 *)((uint8_t *)dst->pixels + y * dst->pitch);
      for (int x = area.x; x < area.x + area.w; x++)
        {
          row[x] = color;
        }
    }
  return 0;
}

// like sdl 1.2: a surface with an alpha channel is blended and its colour
// key ignored, one without one leaves out the pixels of its colour key.
// the screen has no alpha, so blending just mixes the colours
int
SDL_BlitSurface (SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst,
                 SDL_Rect *dstrect)
{
  SDL_Rect from = { 0, 0, src->w, src->h };
  if (srcrect)
    {
      from = *srcrect;
    }
  if (!clip (&from, src))
    {
      return 0;
    }
  SDL_Rect to = { 0, 0, from.w, from.h };
  if (dstrect)
    {
      to.x = dstrect->x;
      to.y = dstrect->y;
    }
  // what clipping cuts off the destination comes off the source too
  int dx = to.x, dy = to.y;
  if (!clip (&to, dst))
    {
      if (dstrect)
        {
          dstrect->w = dstrect->h = 0;
        }
      return 0;
    }
  from.x += to.x - dx;
  from.y += to.y - dy;
  int alpha = src->flags & SDL_SRCALPHA;
  int keyed = !alpha && (src->flags & SDL_SRCCOLORKEY);
  Uint32 key = src->format->colorkey;
  for (int y = 0; y < to.h; y++)
    {
      const Uint32 *in
          = (const Uint32 *)((uint8_t *)src->pixels
                             + (from.y + y) * src->pitch)
            + from.x;
      Uint32 *out
          = (Uint32 *)((uint8_t *)dst->pixels + (to.y + y) * dst->pitch)
            + to.x;
      for (int x = 0; x < to.w; x++)
        {
          Uint32 pixel = in[x];
          if (alpha)
            {
              Uint32 a = pixel >> 24;
              if (a == 0)
                {
                  continue;
                }
              if (a < 255)
                {
                  Uint32 back = out[x], mixed = 0;
                  for (int shift = 0; shift < 24; shift += 8)
                    {
                      Uint32 c = (pixel >> shift & 0xFF) * a
                                 + (back >> shift & 0xFF) * (255 - a);
                      mixed |= c / 255 << shift;
                    }
                  pixel = mixed;
                }
              out[x] = pixel & 0xFFFFFF;
            }
          else if (!keyed || pixel != key)
            {
              out[x] = pixel;
            }
        }
    }
  if (dstrect)
    {
      *dstrect = to;
    }
  return 0;
}

// moos key codes as sdl ones, letters, digits and punctuation are their
// own chars
static SDLKey
sdl_key (int key)
{
  switch (key)
    {
    case MOOS_KEY_UP:
      return SDLK_UP;
    case MOOS_KEY_DOWN:
      return SDLK_DOWN;
    case MOOS_KEY_LEFT:
      return SDLK_LEFT;
    case MOOS_KEY_RIGHT:
      return SDLK_RIGHT;
    case MOOS_KEY_HOME:
      return SDLK_HOME;
    case MOOS_KEY_END:
      return SDLK_END;
    case MOOS_KEY_DELETE:
      return SDLK_DELETE;
    case MOOS_KEY_PAGE_UP:
      return SDLK_PAGEUP;
    case MOOS_KEY_PAGE_DOWN:
      return SDLK_PAGEDOWN;
    case MOOS_KEY_INSERT:
      return SDLK_INSERT;
    case MOOS_KEY_CTRL:
      return SDLK_LCTRL;
    case MOOS_KEY_SHIFT:
      return SDLK_LSHIFT;
    case MOOS_KEY_ALT:
      return SDLK_LALT;
    case '\n':
      return SDLK_RETURN;
    }
  return key < 0x80 ? (SDLKey)key : SDLK_UNKNOWN;
}

int
SDL_PollEvent (SDL_Event *event)
{
  int moos;
  while ((moos = key_event (0)) >= 0)
    {
      int released = moos & MOOS_KEY_RELEASED;
      SDLKey key = sdl_key (moos & ~MOOS_KEY_RELEASED);
      SDLMod mod = key == SDLK_LSHIFT  ? KMOD_LSHIFT
                   : key == SDLK_LCTRL ? KMOD_LCTRL
                   : key == SDLK_LALT  ? KMOD_LALT
                                       : KMOD_NONE;
      mods = released ? mods & ~mod : mods | mod;
      if (key == SDLK_UNKNOWN)
        {
          continue;
        }
      memset (event, 0, sizeof (*event));
      event->type = released ? SDL_KEYUP : SDL_KEYDOWN;
      event->key.state = !released;
      event->key.keysym.sym = key;
      event->key.keysym.mod = mods;
      event->key.keysym.unicode = key < 0x80 ? key : 0;
      return 1;
    }
  return 0;
}

int
SDL_NumJoysticks (void)
{
  return 0;
}

SDL_Joystick *
SDL_JoystickOpen (int index)
{
  return 0;
}

const char *
SDL_JoystickName (int index)
{
  return 0;
}

int
SDL_JoystickNumAxes (SDL_Joystick *joystick)
{
  return 0;
}

int
SDL_JoystickNumButtons (SDL_Joystick *joystick)
{
  return 0;
}

int
SDL_JoystickNumBalls (SDL_Joystick *joystick)
{
  return 0;
}

Uint32
SDL_GetTicks (void)
{
  return uptime ();
}

void
SDL_Delay (Uint32 ms)
{
  sleep (ms);
}

void
SDL_initFramerate (FPSmanager *manager)
{
  manager->rate = 30;
  manager->next = uptime ();
}

int
SDL_setFramerate (FPSmanager *manager, Uint32 rate)
{
  if (rate == 0)
    {
      return -1;
    }
  manager->rate = rate;
  return 0;
}

void
SDL_framerateDelay (FPSmanager *manager)
{
  uint32_t now = uptime ();
  manager->next += 1000 / manager->rate;
  // a frame that took too long doesnt make the next ones hurry
  if ((int32_t)(manager->next - now) < 0)
    {
      manager->next = now;
    }
  sleep_until (manager->next);
}
