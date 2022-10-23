/* scroll.h
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "types.h"


// -----------------------------------------------------------------------------
// scroll
// -----------------------------------------------------------------------------

struct ui_scroll_style
{
    struct rgba fg, bg;
};

struct ui_scroll
{
    struct ui_widget w;
    struct ui_scroll_style s;

    int16_t row_h;
    size_t first, total, visible;
    struct { int16_t start, bar; } drag;
};

struct ui_scroll ui_scroll_new(struct dim dim, int16_t row_h);
void ui_scroll_free(struct ui_scroll *);

void ui_scroll_move(struct ui_scroll *, ssize_t inc);
void ui_scroll_update(struct ui_scroll *, size_t total);

enum ui_ret ui_scroll_event(struct ui_scroll *, const SDL_Event *);
struct ui_layout ui_scroll_render(struct ui_scroll *, struct ui_layout *, SDL_Renderer *);

inline size_t ui_scroll_first(const struct ui_scroll *scroll) { return scroll->first; }
inline size_t ui_scroll_last(const struct ui_scroll *scroll)
{
    return legion_min(scroll->total, scroll->first + scroll->visible);
}
