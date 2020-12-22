/* ui.c
   Rémi Attab (remi.attab@gmail.com), 07 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui.h"
#include "utils/bits.h"
#include "utils/sdl.h"


// -----------------------------------------------------------------------------
// ui_toggle
// -----------------------------------------------------------------------------

void ui_toggle_size(struct font *font, size_t str_len, int *width, int *height)
{
    if (width) *width = font->glyph_w * (str_len + 2);
    if (height) *height = font->glyph_h;
}

void ui_toggle_init(
        struct ui_toggle *toggle,
        const struct SDL_Rect *rect,
        const char * str, size_t str_len)
{
    toggle->rect = *rect;
    toggle->hover = sdl_rect_contains(rect, &core.cursor.point);

    assert(str_len < sizeof(toggle->str) - 1);
    memcpy(toggle->str, str, str_len);
    toggle->str[str_len] = 0;
    toggle->str_len = str_len;
}

void ui_toggle_render(
        struct ui_toggle *toggle,
        SDL_Renderer *renderer,
        SDL_Point pos,
        struct font *font)
{
    font_reset(font);

    if (toggle->hover && !toggle->disabled) {
        const uint8_t gray = 0x18;
        sdl_err(SDL_SetRenderDrawColor(renderer, gray, gray, gray, SDL_ALPHA_OPAQUE));
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = pos.x, .y = pos.y,
                            .w = toggle->rect.w, .h = toggle->rect.h}));
    }

    size_t font_w = font->glyph_w;
    const char *prefix = toggle->selected ? "- " : "+ ";

    if (toggle->disabled) {
        const uint8_t gray = 0x55;
        sdl_err(SDL_SetTextureColorMod(font->tex, gray, gray, gray));
    }

    font_render(font, renderer, prefix, 2, pos);

    pos.x += font_w * 2;
    font_render(font, renderer, toggle->str, toggle->str_len, pos);
}

enum ui_toggle_ret ui_toggle_events(struct ui_toggle *toggle, SDL_Event *event)
{
    switch (event->type) {

    case SDL_MOUSEMOTION: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&toggle->rect, &point)) {
            if (toggle->hover) {
                toggle->hover = false;
                return ui_toggle_invalidate;
            }
            return ui_toggle_nil;
        }

        if (!toggle->hover) {
            toggle->hover = true;
            return ui_toggle_invalidate;
        }
        return ui_toggle_nil;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&toggle->rect, &point)) return ui_toggle_nil;
        if (toggle->disabled) return ui_toggle_consume;

        toggle->selected = !toggle->selected;
        return ui_toggle_flip | ui_toggle_consume | ui_toggle_invalidate;
    }

    default: { return ui_toggle_nil; }
    }

}


// -----------------------------------------------------------------------------
// scroll
// -----------------------------------------------------------------------------

void ui_scroll_init(
        struct ui_scroll *scroll,
        const SDL_Rect *active,
        const SDL_Rect *render,
        size_t total, size_t visible)
{
    *scroll = (struct ui_scroll) {
        .events = *active,
        .render = (SDL_Rect) {
            .x = active->x + render->x,
            .y = active->y + render->y,
            .w = render->w, .h = render->h,
        },

        .total = total,
        .first = 0,
        .visible = visible,

        .drag = {0},
    };
}

void ui_scroll_update(struct ui_scroll *scroll, size_t total)
{
    scroll->first = 0;
    scroll->total = total;
}

static SDL_Rect ui_scroll_bar(struct ui_scroll *scroll, SDL_Point pos)
{
    return (SDL_Rect) {
        .x = pos.x,
        .y = (scroll->render.h * scroll->first) / scroll->total + pos.y,
        .w = scroll->render.w,
        .h = (scroll->render.h * scroll->visible) / scroll->total,
    };
}

void ui_scroll_render(
        struct ui_scroll *scroll, SDL_Renderer *renderer, SDL_Point pos)
{
    if (scroll->visible >= scroll->total) return;

    const uint8_t gray = 0xCC;
    sdl_err(SDL_SetRenderDrawColor(renderer, gray, gray, gray, 0xFF));

    SDL_Rect bar = ui_scroll_bar(scroll, pos);
    sdl_err(SDL_RenderFillRect(renderer, &bar));
}

enum ui_scroll_ret ui_scroll_events(struct ui_scroll *scroll, SDL_Event *event)
{
    switch (event->type) {

    case SDL_MOUSEWHEEL: {
        if (!sdl_rect_contains(&scroll->events, &core.cursor.point))
            return ui_scroll_nil;

        ssize_t first = (ssize_t) scroll->first - event->wheel.y;
        if (first < 0) return ui_scroll_consume;
        if (first + scroll->visible > scroll->total) return ui_scroll_consume;
        if (!event->wheel.y) return ui_scroll_consume;

        scroll->first = (size_t) first;
        return ui_scroll_moved | ui_scroll_invalidate | ui_scroll_consume;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Rect bar = ui_scroll_bar(scroll,
                (SDL_Point) { .x = scroll->render.x, .y = scroll->render.y });

        dbg("scroll.event> cursor:{%d, %d}, bar:{%d, %d, %d, %d}, render:{%d, %d, %d, %d}, active:{%d, %d, %d, %d}",
                core.cursor.point.x, core.cursor.point.y,
                bar.x, bar.y, bar.w, bar.h,
                scroll->render.x, scroll->render.y,
                scroll->render.w, scroll->render.h,
                scroll->events.x, scroll->events.y,
                scroll->events.w, scroll->events.h);
        if (!sdl_rect_contains(&bar, &core.cursor.point))
            return ui_scroll_nil;

        scroll->drag.y = core.cursor.point.y;
        scroll->drag.top = bar.y - scroll->render.y;
        return ui_scroll_consume;
    }

    case SDL_MOUSEBUTTONUP: {
        if (!scroll->drag.y) return ui_scroll_nil;
        scroll->drag.y = 0;
        return ui_scroll_consume;
    }

    case SDL_MOUSEMOTION: {
        if (!scroll->drag.y) return ui_scroll_nil;

        int delta = core.cursor.point.y - scroll->drag.y;
        int top = scroll->drag.top + delta;
        ssize_t first = (top * scroll->total) / scroll->render.h;

        if (first < 0) return ui_scroll_consume;
        if ((size_t) first + scroll->visible > scroll->total) return ui_scroll_consume;
        if ((size_t) first == scroll->first) return ui_scroll_consume;

        scroll->first = (size_t) first;
        return ui_scroll_moved | ui_scroll_invalidate | ui_scroll_consume;
    }

    default: { return ui_scroll_nil; }
    }
}
