/* font.h
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "SDL.h"

// -----------------------------------------------------------------------------
// font
// -----------------------------------------------------------------------------

enum {
    charmap_start = 33,
    charmap_end = 127,
    charmap_len = charmap_end - charmap_start,
};

struct font
{
    size_t glyph_w;
    size_t glyph_h;
    size_t glyph_baseline;
    SDL_Texture *tex;
};

extern struct font *font_mono4;
extern struct font *font_mono6;
extern struct font *font_mono8;


void fonts_init(SDL_Renderer *);
void fonts_close();

struct font *font_open(SDL_Renderer *, const char *ttf, size_t px);
void font_close(struct font *);

void font_reset(struct font *);
void font_text_size(struct font *, size_t len, size_t *w, size_t *h);
void font_render(struct font *, SDL_Renderer *, const char *str, size_t len, SDL_Point);
