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

SDL_Texture *db_img_cursor(SDL_Renderer *);
SDL_Texture *db_img_map(SDL_Renderer *);


// -----------------------------------------------------------------------------
// font
// -----------------------------------------------------------------------------

struct db_font { size_t len; const uint8_t *ptr; };
struct db_font db_font_regular(void);
struct db_font db_font_italic(void);
struct db_font db_font_bold(void);


// -----------------------------------------------------------------------------
// man
// -----------------------------------------------------------------------------

struct db_man_it
{
    const void *it, *end;
    size_t path_len; const char *path;
    size_t data_len; const char *data;
};

bool db_man_next(struct db_man_it *);
