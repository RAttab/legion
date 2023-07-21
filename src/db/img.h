/* img.h
   Rémi Attab (remi.attab@gmail.com), 21 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "SDL.h"


// -----------------------------------------------------------------------------
// img
// -----------------------------------------------------------------------------

SDL_Texture *img_cursor(SDL_Renderer *);
SDL_Texture *img_map(SDL_Renderer *);
