/* types.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/color.h"

#include "SDL.h"

struct font;
struct ui_style;
struct ui_panel;
struct ui_layout;


// -----------------------------------------------------------------------------
// ret
// -----------------------------------------------------------------------------

enum ui_ret
{
    ui_nil = 0,
    ui_skip = 1 << 0,
    ui_action = 1 << 1,
    ui_consume = 1 << 2,
};


// -----------------------------------------------------------------------------
// pos
// -----------------------------------------------------------------------------

struct pos { int16_t x, y; };

inline struct pos make_pos(int16_t x, int16_t y)
{
    return (struct pos) { .x = x, .y = y };
}

inline struct pos make_pos_from_rect(SDL_Rect rect)
{
    return make_pos(rect.x, rect.y);
}

inline SDL_Point pos_as_point(struct pos pos)
{
    return (SDL_Point) { .x = pos.x, .y = pos.y };
}

inline bool pos_is_nil(struct pos pos) { return !pos.x && !pos.y; }


// -----------------------------------------------------------------------------
// dim
// -----------------------------------------------------------------------------

struct dim { int16_t w, h; };

inline struct dim make_dim(int16_t w, int16_t h)
{
    return (struct dim) { .w = w, .h = h };
}

inline struct dim make_dim_from_rect(SDL_Rect rect)
{
    return make_dim(rect.w, rect.h);
}

inline bool dim_is_nil(struct dim dim) { return !dim.w && !dim.h; }


// -----------------------------------------------------------------------------
// widget
// -----------------------------------------------------------------------------

struct ui_widget
{
    struct pos pos;
    struct dim dim;
};

inline struct ui_widget ui_widget_new(int16_t w, int16_t h)
{
    return (struct ui_widget) {
        .dim = (struct dim) { .w = w, .h = h },
    };
}

inline struct SDL_Rect ui_widget_rect(const struct ui_widget *widget)
{
    return (SDL_Rect) {
        .x = widget->pos.x, .y = widget->pos.y,
        .w = widget->dim.w, .h = widget->dim.h,
    };
}
