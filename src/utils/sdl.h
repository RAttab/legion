/* sdl.h
   Rémi Attab (remi.attab@gmail.com), 07 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// sdl
// -----------------------------------------------------------------------------

inline bool sdl_rect_contains(SDL_Rect *rect, SDL_Point *point)
{
    int x = point->x - rect->x;
    int y = point->y - rect->y;
    return
        x >= 0 && x <= rect->w &&
        y >= 0 && y <= rect->h;
}
