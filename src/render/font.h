/* font.h
   Rémi Attab (remi.attab@gmail.com), 14 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "ui/ui.h"
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


enum font_size { font_small = 0, font_big, font_size_max };
enum font_style { font_nil = 0, font_bold, font_italic, font_style_max };
struct font *make_font(enum font_size, enum font_style);

extern struct font *font_mono4;
extern struct font *font_mono6;
extern struct font *font_mono8;
extern struct font *font_mono10;

void fonts_populate(SDL_Renderer *);
void fonts_close();

struct font *font_open(SDL_Renderer *, const char *ttf, size_t px);
void font_close(struct font *);

void font_text_size(struct font *, size_t len, size_t *w, size_t *h);
void font_render(
        struct font *, SDL_Renderer *,
        SDL_Point, struct rgba,
        const char *str, size_t len);
