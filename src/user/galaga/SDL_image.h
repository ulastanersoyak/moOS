#ifndef MOOS_SDL_IMAGE_H
#define MOOS_SDL_IMAGE_H

#include "SDL.h"

// loads a png into a 32 bit surface with an alpha channel, src/user/galaga/png.c
SDL_Surface *IMG_Load (const char *path);

#endif
