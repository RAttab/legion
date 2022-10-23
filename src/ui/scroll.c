/* scroll.c
   Rémi Attab (remi.attab@gmail.com), 12 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "scroll.h"


// -----------------------------------------------------------------------------
// scroll
// -----------------------------------------------------------------------------

enum { ui_scroll_width = 8 };

struct ui_scroll ui_scroll_new(struct dim dim, int16_t row_h)
{
    return (struct ui_scroll) {
        .w = ui_widget_new(dim.w, dim.h),
        .s = ui_st.scroll,
        .row_h = row_h,
        .first = 0,
        .total = 0,
        .visible = 0,
    };
}

void ui_scroll_free(struct ui_scroll *scroll)
{
    (void) scroll;
}

void ui_scroll_move(struct ui_scroll *scroll, ssize_t inc)
{
    scroll->first = legion_bound(
            (ssize_t) scroll->first + inc,
            (ssize_t) 0,
            (ssize_t) scroll->total);
}

void ui_scroll_update(struct ui_scroll *scroll, size_t total)
{
    scroll->total = total;
    scroll->first = legion_min(scroll->first, scroll->total);
}

static SDL_Rect ui_scroll_rect(struct ui_scroll *scroll)
{
    if (!scroll->total) return (SDL_Rect) {0};
    SDL_Rect rect = {
        .x = scroll->w.pos.x + (scroll->w.dim.w - ui_scroll_width),
        .y = scroll->w.pos.y + ((scroll->w.dim.h * scroll->first) / scroll->total),
        .w = ui_scroll_width,
        .h = (scroll->w.dim.h * scroll->visible) / scroll->total,
    };

    const int max = scroll->w.pos.y + scroll->w.dim.h;
    if (rect.y + rect.h > max) rect.h = max - rect.y;

    return rect;
}


enum ui_ret ui_scroll_event(struct ui_scroll *scroll, const SDL_Event *ev)
{
    switch (ev->type) {

    case SDL_MOUSEWHEEL: {
        SDL_Rect widget = ui_widget_rect(&scroll->w);
        if (!sdl_rect_contains(&widget, &render.cursor.point))
            return ui_nil;

        ui_scroll_move(scroll, -ev->wheel.y);
        return ui_consume;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Rect bar = ui_scroll_rect(scroll);
        if (!sdl_rect_contains(&bar, &render.cursor.point))
            return ui_nil;

        scroll->drag.start = render.cursor.point.y;
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

        int16_t delta = render.cursor.point.y - scroll->drag.start;
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

struct ui_layout ui_scroll_render(
        struct ui_scroll *scroll, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_layout_add(layout, &scroll->w);

    if (!rgba_is_nil(scroll->s.bg)) {
        rgba_render(scroll->s.bg, renderer);
        SDL_Rect rect = ui_widget_rect(&scroll->w);
        sdl_err(SDL_RenderFillRect(renderer, &rect));
    }

    scroll->visible = scroll->w.dim.h / scroll->row_h;

    struct dim dim = make_dim(scroll->w.dim.w - ui_scroll_width, scroll->w.dim.h);
    struct ui_layout inner = ui_layout_new(scroll->w.pos, dim);
    if (!scroll->first && scroll->visible >= scroll->total) return inner;

    rgba_render(scroll->s.fg, renderer);
    SDL_Rect rect = ui_scroll_rect(scroll);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    return inner;
}
