/* toggle.c
   Rémi Attab (remi.attab@gmail.com), 12 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/font.h"


// -----------------------------------------------------------------------------
// toggle
// -----------------------------------------------------------------------------

struct ui_toggle ui_toggle_new(struct font *font, struct ui_str str)
{
    return (struct ui_toggle) {
        .w = ui_widget_new(ui_str_len(&str) * font->glyph_w, font->glyph_h),
        .str = str,
        .font = font,
        .state = ui_toggle_idle,
    };
}

void ui_toggle_free(struct ui_toggle *toggle)
{
    ui_str_free(&toggle->str);
}

enum ui_ret ui_toggle_event(struct ui_toggle *toggle, const SDL_Event *ev)
{
    struct SDL_Rect rect = ui_widget_rect(&toggle->w);

    switch (ev->type) {

    case SDL_MOUSEMOTION: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) {
            if (toggle->state == ui_toggle_hover) toggle->state = ui_toggle_idle;
        }
        else if (toggle->state == ui_toggle_idle) toggle->state = ui_toggle_hover;

        return ui_nil;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) return ui_nil;
        toggle->state = toggle->state == ui_toggle_selected ?
            ui_toggle_idle : ui_toggle_selected;
        return ui_consume;
    }

    default: { return ui_nil; }
    }
}

void ui_toggle_render(
        struct ui_toggle *toggle, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_layout_add(layout, &toggle->w);

    switch (toggle->state) {
    case ui_toggle_idle: { rgba_render(rgba_nil(), renderer); break; }
    case ui_toggle_hover: { rgba_render(rgba_gray(0x44), renderer); break; }
    case ui_toggle_selected: { rgba_render(rgba_gray(0x22), renderer); break; }
    default: { assert(false); }
    }

    SDL_Rect rect = ui_widget_rect(&toggle->w);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    SDL_Point point = pos_as_point(toggle->w.pos);
    font_render(toggle->font, renderer, point, rgba_white(), toggle->str.str, toggle->str.len);
}


// -----------------------------------------------------------------------------
// toggles
// -----------------------------------------------------------------------------

struct ui_toggles ui_toggles_new(struct font *font, struct ui_str str)
{
    return (struct ui_toggles) {
        .font = font,
        .str = str,
        .len = 0,
        .cap = 0,
        .items = NULL,
    };
}

void ui_toggles_free(struct ui_toggles *list)
{
    for (size_t i = 0; i < list->cap; ++i)
        ui_toggle_free(&list->items[i]);
    free(list->items);
}


void ui_toggles_resize(struct ui_toggles *list, size_t len)
{
    if (len <= list->cap) {
        list->len = len;
        return;
    }

    size_t cap = list->cap ? list->cap : 8;
    while (cap < len) cap *= 2;

    list->items = realloc(list->items, cap * sizeof(*list->items));
    for (size_t i = list->cap; i < cap; ++i)
        list->items[i] = ui_toggle_new(list->font, ui_str_clone(&list->str));

    list->len = len;
    list->cap = cap;
}

enum ui_ret ui_toggles_event(
        struct ui_toggles *list, const SDL_Event *ev, const struct ui_scroll *scroll,
        struct ui_toggle **r_toggle, size_t *r_index)
{
    size_t first = scroll ? ui_scroll_first(scroll) : 0;
    size_t last = scroll ? ui_scroll_last(scroll) : list->len;

    enum ui_ret final = ui_nil;
    for (size_t i = first; i < last; ++i) {
        enum ui_ret ret = ui_nil;
        if ((ret = ui_toggle_event(&list->items[i], ev))) {
            final = ret;
            if (r_index) *r_index = i;
            if (r_toggle) *r_toggle = &list->items[i];
        }
    }

    return final;
}

void ui_toggles_render(
        struct ui_toggles *list,
        struct ui_layout *layout,
        SDL_Renderer *renderer,
        const struct ui_scroll *scroll)
{
    size_t first = scroll ? ui_scroll_first(scroll) : 0;
    size_t last = scroll ? ui_scroll_last(scroll) : list->len;

    for (size_t i = first; i < last; ++i) {
        ui_toggle_render(&list->items[i], layout, renderer);
        ui_layout_next_row(layout);
    }
}

void ui_toggles_clear(struct ui_toggles *list)
{
    for (size_t i = 0; i < list->len; ++i)
        list->items[i].state = ui_toggle_idle;
}

void ui_toggles_select(struct ui_toggles *list, uint64_t user)
{
    for (size_t i = 0; i < list->len; ++i) {
        struct ui_toggle *toggle = list->items + i;
        toggle->state = toggle->user == user ? ui_toggle_selected : ui_toggle_idle;
    }
}
