/* toggle.c
   Rémi Attab (remi.attab@gmail.com), 12 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"


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

struct toggle *toggle_set(struct toggle *toggle, const char *str, size_t len)
{
    assert((void *)(toggle + 1) == (void *)toggle->str);
    assert(len <= toggle->len);
    memcpy(toggle->str, str, len);
}

enum ui_ret toggle_event(struct toggle *toggle, const SDL_Event *ev)
{
    struct SDL_Rect rect = widget_rect(&toggle->w);

    switch (event->type) {

    case SDL_MOUSEMOTION: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) {
            if (toggle->state == toggle_hover) toggle->state = toggle_idle;
            return ui_nil;
        }
        toggle->state = toggle_hover;
        return ui_nil;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) return ui_nil;
        toggle->state = toggle_selected;
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
    case toggle_hover: { rgba_render(rgba_gray(0xAA), renderer); break; }
    case toggle_selected: { rgba_render(rgba_gray(0x66), renderer); break; }
    default: { assert(false); }
    }
    sdl_err(SDL_RenderFillRect(renderer, &wdidget_rect(&toggle->w)));

    font_render(toggle->font, layout->renderer, toggle->fg, toggle->w.pos, str, len);
}
