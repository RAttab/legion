/* button.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"


// -----------------------------------------------------------------------------
// button
// -----------------------------------------------------------------------------

struct button *button_const(struct font *font, const char *str)
{
    size_t len = strnlen(str, button_cap);
    struct button *button = calloc(1, sizeof(*button));

    *button = (struct button) {
        .w = make_widget(len * font->glyph_w, font->glyph_h),
        .font = font,
        .fg = rgba_white(),
        .state = button_idle,
        .len = len,
        .str = str,
    };
    return button;
}

struct button *button_var(struct font *font, size_t len)
{
    assert(len <= button_cap);

    struct button *button = calloc(1, sizeof(*button) + len);
    *button = (struct button) {
        .w = make_widget(len * font->glyph_w, font->glyph_h),
        .font = font,
        .fg = rgba_white(),
        .state = button_idle,
        .len = len,
        .str = (void *)(button + 1),
    };
    return button;
}

struct button *button_set(struct button *button, const char *str, size_t len)
{
    assert((void *)(button + 1) == (void *)button->str);
    assert(len <= button->len);
    memcpy(button->str, str, len);
}

enum ui_ret button_event(struct button *button, const SDL_Event *ev)
{
    struct SDL_Rect rect = widget_rect(&button->w);

    switch (event->type) {

    case SDL_MOUSEMOTION: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) {
            button->state = button_idle;
            return ui_nil;
        }
        button->state = button_hover;
        return ui_nil;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) return ui_nil;
        button->state = button_pressed;
        return ui_consume;
    }

    default: { return ui_nil; }
    }
}

void button_render(
        struct button *button, struct layout *layout, SDL_Renderer *renderer)
{
    layout_add(layout, &button->w);

    switch (button->state) {
    case button_idle: { rgba_render(rgba_gray(0x88), renderer); break; }
    case button_hover: { rgba_render(rgba_gray(0xAA), renderer); break; }
    case button_pressed: { rgba_render(rgba_gray(0x44), renderer); break; }
    default: { assert(false); }
    }
    sdl_err(SDL_RenderFillRect(renderer, &wdidget_rect(&button->w)));

    font_render(button->font, layout->renderer, button->fg, button->w.pos, str, len);
}
