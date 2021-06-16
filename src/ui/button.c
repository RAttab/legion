/* button.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/core.h"
#include "render/font.h"
#include "utils//sdl.h"


// -----------------------------------------------------------------------------
// button
// -----------------------------------------------------------------------------

struct button *button_const(struct font *font, const char *str)
{
    size_t len = strnlen(str, button_cap);
    struct button *button = calloc(1, sizeof(*button));

    struct dim pad = make_dim(6, 2);
    *button = (struct button) {
        .w = make_widget(
                font->glyph_w * len + pad.w*2,
                font->glyph_h + pad.h*2),
        .font = font,
        .fg = rgba_white(),
        .pad = pad,
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

void button_free(struct button *button)
{
    free(button);
}

void button_set(struct button *button, const char *str, size_t len)
{
    assert((void *)(button + 1) == (void *)button->str);
    assert(len <= button->len);
    memcpy(button + 1, str, len);
}

enum ui_ret button_event(struct button *button, const SDL_Event *ev)
{
    struct SDL_Rect rect = widget_rect(&button->w);

    switch (ev->type) {

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

    case SDL_MOUSEBUTTONUP: {
        SDL_Point point = core.cursor.point;
        button->state = sdl_rect_contains(&rect, &point) ?
            button_hover : button_idle;
        return ui_nil;
    }

    default: { return ui_nil; }
    }
}

void button_render(
        struct button *button, struct layout *layout, SDL_Renderer *renderer)
{
    layout_add(layout, &button->w);

    switch (button->state) {
    case button_idle: { rgba_render(rgba_gray(0x22), renderer); break; }
    case button_hover: { rgba_render(rgba_gray(0x33), renderer); break; }
    case button_pressed: { rgba_render(rgba_gray(0x11), renderer); break; }
    default: { assert(false); }
    }

    SDL_Rect rect = widget_rect(&button->w);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    SDL_Point point = {
        .x = button->w.pos.x + button->pad.w,
        .y = button->w.pos.y + button->pad.h
    };
    font_render(button->font, renderer, point, button->fg, button->str, button->len);
}
