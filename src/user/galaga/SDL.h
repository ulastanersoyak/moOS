#ifndef MOOS_SDL_H
#define MOOS_SDL_H

#include <stdint.h>
// like SDL_stdinc.h, programs get the c library with sdl
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// the part of sdl 1.2 that xgalaga uses, on top of the moos graphics, key
// event and timer system calls. src/user/galaga.c has the code. surfaces are
// always 32 bits per pixel, the screen is drawn at the size the game asks
// for and shrunk into vga mode 13h when it is flipped

#define SDL_MAJOR_VERSION 1
#define SDL_MINOR_VERSION 2
#define SDL_PATCHLEVEL 15
#define SDL_VERSIONNUM(x, y, z) ((x) * 1000 + (y) * 100 + (z))
#define SDL_VERSION_ATLEAST(x, y, z)                                          \
  (SDL_VERSIONNUM (SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL)     \
   >= SDL_VERSIONNUM (x, y, z))

typedef uint8_t Uint8;
typedef int8_t Sint8;
typedef uint16_t Uint16;
typedef int16_t Sint16;
typedef uint32_t Uint32;
typedef int32_t Sint32;

#define SDL_INIT_AUDIO 0x10
#define SDL_INIT_VIDEO 0x20
#define SDL_INIT_JOYSTICK 0x200

#define SDL_SWSURFACE 0x0
#define SDL_SRCCOLORKEY 0x1000
#define SDL_SRCALPHA 0x10000
#define SDL_FULLSCREEN 0x80000000
#define SDL_DISABLE 0
#define SDL_ENABLE 1
#define SDL_MUSTLOCK(surface) 0

typedef struct SDL_PixelFormat
{
  Uint8 BitsPerPixel;
  Uint8 BytesPerPixel;
  Uint8 Rshift, Gshift, Bshift, Ashift;
  Uint32 Rmask, Gmask, Bmask, Amask;
  Uint32 colorkey;
} SDL_PixelFormat;

typedef struct SDL_Surface
{
  Uint32 flags;
  SDL_PixelFormat *format;
  int w, h;
  Uint16 pitch;
  void *pixels;
} SDL_Surface;

typedef struct SDL_Rect
{
  Sint16 x, y;
  Uint16 w, h;
} SDL_Rect;

typedef struct SDL_Color
{
  Uint8 r, g, b, unused;
} SDL_Color;

int SDL_Init (Uint32 flags);
int SDL_InitSubSystem (Uint32 flags);
void SDL_Quit (void);
char *SDL_GetError (void);
int SDL_ShowCursor (int toggle);
void SDL_WM_SetCaption (const char *title, const char *icon);

int SDL_VideoModeOK (int width, int height, int bpp, Uint32 flags);
SDL_Surface *SDL_SetVideoMode (int width, int height, int bpp, Uint32 flags);
SDL_Surface *SDL_DisplayFormat (SDL_Surface *surface);
// only 32 bits per pixel, Amask 0 makes a surface without alpha
SDL_Surface *SDL_CreateRGBSurface (Uint32 flags, int width, int height,
                                   int depth, Uint32 Rmask, Uint32 Gmask,
                                   Uint32 Bmask, Uint32 Amask);
int SDL_Flip (SDL_Surface *screen);
void SDL_FreeSurface (SDL_Surface *surface);
int SDL_LockSurface (SDL_Surface *surface);
void SDL_UnlockSurface (SDL_Surface *surface);
int SDL_SetColorKey (SDL_Surface *surface, Uint32 flag, Uint32 key);
Uint32 SDL_MapRGB (const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b);
Uint32 SDL_MapRGBA (const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b,
                    Uint8 a);
int SDL_FillRect (SDL_Surface *dst, SDL_Rect *rect, Uint32 color);
int SDL_BlitSurface (SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst,
                     SDL_Rect *dstrect);

// key codes are sdl 1.2's: ascii for the keys that have a char
typedef enum
{
  SDLK_UNKNOWN = 0,
  SDLK_BACKSPACE = 8,
  SDLK_TAB = 9,
  SDLK_RETURN = 13,
  SDLK_ESCAPE = 27,
  SDLK_SPACE = 32,
  SDLK_0 = 48,
  SDLK_9 = 57,
  SDLK_a = 97,
  SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f, SDLK_g, SDLK_h, SDLK_i, SDLK_j,
  SDLK_k, SDLK_l, SDLK_m, SDLK_n, SDLK_o, SDLK_p, SDLK_q, SDLK_r, SDLK_s,
  SDLK_t, SDLK_u, SDLK_v, SDLK_w, SDLK_x, SDLK_y, SDLK_z,
  SDLK_DELETE = 127,
  SDLK_KP4 = 260,
  SDLK_KP_ENTER = 271,
  SDLK_UP = 273,
  SDLK_DOWN = 274,
  SDLK_RIGHT = 275,
  SDLK_LEFT = 276,
  SDLK_INSERT = 277,
  SDLK_HOME = 278,
  SDLK_END = 279,
  SDLK_PAGEUP = 280,
  SDLK_PAGEDOWN = 281,
  SDLK_RSHIFT = 303,
  SDLK_LSHIFT = 304,
  SDLK_RCTRL = 305,
  SDLK_LCTRL = 306,
  SDLK_RALT = 307,
  SDLK_LALT = 308,
} SDLKey;

typedef enum
{
  KMOD_NONE = 0,
  KMOD_LSHIFT = 0x1,
  KMOD_RSHIFT = 0x2,
  KMOD_LCTRL = 0x40,
  KMOD_RCTRL = 0x80,
  KMOD_LALT = 0x100,
  KMOD_RALT = 0x200,
} SDLMod;

typedef struct SDL_keysym
{
  Uint8 scancode;
  SDLKey sym;
  SDLMod mod;
  Uint16 unicode;
} SDL_keysym;

enum
{
  SDL_NOEVENT = 0,
  SDL_KEYDOWN = 2,
  SDL_KEYUP = 3,
  SDL_JOYAXISMOTION = 7,
  SDL_JOYHATMOTION = 9,
  SDL_JOYBUTTONDOWN = 10,
  SDL_JOYBUTTONUP = 11,
  SDL_QUIT = 12,
};

typedef struct SDL_KeyboardEvent
{
  Uint8 type;
  Uint8 which;
  Uint8 state;
  SDL_keysym keysym;
} SDL_KeyboardEvent;

typedef struct SDL_JoyAxisEvent
{
  Uint8 type;
  Uint8 which;
  Uint8 axis;
  Sint16 value;
} SDL_JoyAxisEvent;

typedef struct SDL_JoyHatEvent
{
  Uint8 type;
  Uint8 which;
  Uint8 hat;
  Uint8 value;
} SDL_JoyHatEvent;

typedef struct SDL_JoyButtonEvent
{
  Uint8 type;
  Uint8 which;
  Uint8 button;
  Uint8 state;
} SDL_JoyButtonEvent;

typedef union SDL_Event
{
  Uint8 type;
  SDL_KeyboardEvent key;
  SDL_JoyAxisEvent jaxis;
  SDL_JoyHatEvent jhat;
  SDL_JoyButtonEvent jbutton;
} SDL_Event;

int SDL_PollEvent (SDL_Event *event);

// moos has no joystick, SDL_NumJoysticks is always 0
#define SDL_HAT_CENTERED 0x0
#define SDL_HAT_UP 0x1
#define SDL_HAT_RIGHT 0x2
#define SDL_HAT_DOWN 0x4
#define SDL_HAT_LEFT 0x8
typedef struct SDL_Joystick SDL_Joystick;
int SDL_NumJoysticks (void);
SDL_Joystick *SDL_JoystickOpen (int index);
const char *SDL_JoystickName (int index);
int SDL_JoystickNumAxes (SDL_Joystick *joystick);
int SDL_JoystickNumButtons (SDL_Joystick *joystick);
int SDL_JoystickNumBalls (SDL_Joystick *joystick);

Uint32 SDL_GetTicks (void);
void SDL_Delay (Uint32 ms);

// SDL_endian.h, the i386 is little endian
#define SDL_SwapBE32(x) __builtin_bswap32 (x)
#define SDL_SwapBE16(x) __builtin_bswap16 (x)

#endif
