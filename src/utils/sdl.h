/* sdl.h
   Rémi Attab (remi.attab@gmail.com), 07 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

#include "SDL.h"


// -----------------------------------------------------------------------------
// sdl
// -----------------------------------------------------------------------------

inline bool sdl_rect_contains(const SDL_Rect *rect, const SDL_Point *point)
{
    int x = point->x - rect->x;
    int y = point->y - rect->y;
    return
        x >= 0 && x < rect->w &&
        y >= 0 && y < rect->h;
}

inline void sdl_disable_signals(void)
{
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
}
