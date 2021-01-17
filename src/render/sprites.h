/* sprites.h
   Rémi Attab (remi.attab@gmail.com), 17 Jan 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

#include "SDL.h"


// -----------------------------------------------------------------------------
// sprites
// -----------------------------------------------------------------------------

struct sprites
{
    size_t len;
    size_t w, h;
    size_t rows, cols;
    SDL_Texture *tex;
};

struct sprites *sprites_items;

void sprites_init(SDL_Renderer *renderer);

struct sprites *sprites_open(SDL_Renderer *, const char *path, size_t w, size_t h);
void sprites_close(struct sprites *);
void sprites_reset(struct sprites *);
void sprites_render(struct sprites *, SDL_Renderer *, size_t index, SDL_Rect *dst);
