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

struct toggle *toggle_const(struct font *font, const char *str)
{
    size_t len = strnlen(str, toggle_cap);
    struct toggle *toggle = calloc(1, sizeof(*toggle));

    *toggle = (struct toggle) {
        .w = make_widget(len * font->glyph_w, font->glyph_h),
        .font = font,
        .fg = rgba_white(),
        .state = toggle_idle,
        .len = len,
        .str = str,
    };
    return toggle;
}

struct toggle *toggle_var(struct font *font, size_t len)
{
    assert(len <= toggle_cap);

    struct toggle *toggle = calloc(1, sizeof(*toggle) + len);
    *toggle = (struct toggle) {
        .w = make_widget(len * font->glyph_w, font->glyph_h),
        .font = font,
        .fg = rgba_white(),
        .state = toggle_idle,
        .len = len,
        .str = (void *)(toggle + 1),
    };
    return toggle;
}

void toggle_free(struct toggle *toggle)
{
    free(toggle);
}

void toggle_set(struct toggle *toggle, const char *str, size_t len)
{
    assert((void *)(toggle + 1) == (void *)toggle->str);
    assert(len <= toggle->len);
    memcpy(toggle + 1, str, len);
}

enum ui_ret toggle_event(struct toggle *toggle, const SDL_Event *ev)
{
    struct SDL_Rect rect = widget_rect(&toggle->w);

    switch (ev->type) {

    case SDL_MOUSEMOTION: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) {
            if (toggle->state == toggle_hover) toggle->state = toggle_idle;
        }
        else if (toggle->state == toggle_idle) toggle->state = toggle_hover;

        return ui_nil;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) return ui_nil;
        toggle->state = toggle->state == toggle_selected ?
            toggle_idle : toggle_selected;
        return ui_consume;
    }

    default: { return ui_nil; }
    }
}

void toggle_render(
        struct toggle *toggle, struct layout *layout, SDL_Renderer *renderer)
{
    layout_add(layout, &toggle->w);

    switch (toggle->state) {
    case toggle_idle: { rgba_render(rgba_nil(), renderer); break; }
    case toggle_hover: { rgba_render(rgba_gray(0x44), renderer); break; }
    case toggle_selected: { rgba_render(rgba_gray(0x22), renderer); break; }
    default: { assert(false); }
    }

    SDL_Rect rect = widget_rect(&toggle->w);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    SDL_Point point = pos_as_point(toggle->w.pos);
    font_render(toggle->font, renderer, point, toggle->fg, toggle->str, toggle->len);
}
