#ifndef MOOS_SDL_FRAMERATE_H
#define MOOS_SDL_FRAMERATE_H

#include "SDL.h"

// sdl_gfx's frame rate manager: SDL_framerateDelay sleeps until the next
// frame is due
typedef struct FPSmanager
{
  Uint32 rate;
  Uint32 next; // uptime when the next frame is due
} FPSmanager;

void SDL_initFramerate (FPSmanager *manager);
int SDL_setFramerate (FPSmanager *manager, Uint32 rate);
void SDL_framerateDelay (FPSmanager *manager);

#endif
