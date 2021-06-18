/* scroll.c
   Rémi Attab (remi.attab@gmail.com), 12 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/font.h"


// -----------------------------------------------------------------------------
// scroll
// -----------------------------------------------------------------------------

enum { scroll_width = 8 };

struct scroll *scroll_new(struct dim dim, size_t total, size_t visible)
{
    assert(visible > 0);

    struct scroll *scroll = calloc(1, sizeof(*scroll));
    *scroll = (struct scroll) {
        .w = make_widget(dim.w, dim.h),
        .first = 0,
        .total = total,
        .visible = visible,
    };

    return scroll;
}

void scroll_free(struct scroll *scroll)
{
    free(scroll);
}

void scroll_move(struct scroll *scroll, ssize_t inc)
{
    if (scroll->total <= scroll->visible) return;
    if (inc > 0 || scroll->first) scroll->first += inc;
    scroll->first = legion_min(scroll->first, scroll->total - 1);
}

void scroll_update(struct scroll *scroll, size_t total)
{
    scroll->total = total;
    scroll->first = legion_min(scroll->first, scroll->total);
}

static SDL_Rect scroll_rect(struct scroll *scroll)
{
    if (!scroll->total) return (SDL_Rect) {0};
    return (SDL_Rect) {
        .x = scroll->w.pos.x + (scroll->w.dim.w - scroll_width),
        .y = scroll->w.pos.y + ((scroll->w.dim.h * scroll->first) / scroll->total),
        .w = scroll_width,
        .h = (scroll->w.dim.h * scroll->visible) / scroll->total,
    };
}


enum ui_ret scroll_event(struct scroll *scroll, const SDL_Event *ev)
{
    switch (ev->type) {

    case SDL_MOUSEWHEEL: {
        SDL_Rect widget = widget_rect(&scroll->w);
        if (!sdl_rect_contains(&widget, &core.cursor.point))
            return ui_nil;

        scroll_move(scroll, -ev->wheel.y);
        return ui_consume;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Rect bar = scroll_rect(scroll);
        if (!sdl_rect_contains(&bar, &core.cursor.point))
            return ui_nil;

        scroll->drag.start = core.cursor.point.y;
        scroll->drag.bar = bar.y;
        return ui_consume;
    }

    case SDL_MOUSEBUTTONUP: {
        if (!scroll->drag.start) return ui_nil;
        memset(&scroll->drag, 0, sizeof(scroll->drag));
        return ui_consume;
    }

    case SDL_MOUSEMOTION: {
        if (!scroll->drag.start) return ui_nil;

        int16_t delta = core.cursor.point.y - scroll->drag.start;
        int16_t bar = scroll->drag.bar + delta;
        if (bar < scroll->w.pos.y) scroll->first = 0;
        else {
            size_t first = ((bar - scroll->w.pos.y) * scroll->total) / scroll->w.dim.h;
            scroll->first = legion_min(scroll->total, first);
        }

        return ui_nil;
    }

    default: { return ui_nil; }
    }
}

struct layout scroll_render(
        struct scroll *scroll, struct layout *layout, SDL_Renderer *renderer)
{
    layout_add(layout, &scroll->w);

    struct dim dim = make_dim(scroll->w.dim.w - scroll_width, scroll->w.dim.h);
    struct layout inner = layout_new(scroll->w.pos, dim);
    if (scroll->visible >= scroll->total) return inner;

    rgba_render(rgba_white(), renderer);

    SDL_Rect rect = scroll_rect(scroll);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    return inner;
}
