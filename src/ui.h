/* ui.c
   Rémi Attab (remi.attab@gmail.com), 04 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "SDL.h"

#include "coord.h"

struct sector;

struct ui_core
{
    struct sector *sector;
    struct coord pos;
    scale_t scale;

    SDL_Rect rect;
    SDL_Texture* tex;

    struct ui_cursor *cursor;
    struct panel * p_coord;

    struct system *selected;
};

struct ui_core *ui_core_init(SDL_Renderer *, struct sector *, SDL_Rect *);
void ui_core_free(struct ui_core *);

void ui_core_render(struct ui_core *, SDL_Renderer *);
void ui_core_events(struct ui_core *, SDL_Event *);

