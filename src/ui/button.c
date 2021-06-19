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

struct ui_button *ui_button_const(struct font *font, const char *str)
{
    size_t len = strnlen(str, ui_button_cap);
    struct ui_button *button = calloc(1, sizeof(*button));

    struct dim pad = make_dim(6, 2);
    *button = (struct ui_button) {
        .w = ui_widget_new(
                font->glyph_w * len + pad.w*2,
                font->glyph_h + pad.h*2),
        .font = font,
        .fg = rgba_white(),
        .pad = pad,
        .state = ui_button_idle,
        .len = len,
        .str = str,
    };
    return button;
}

struct ui_button *ui_button_var(struct font *font, size_t len)
{
    assert(len <= ui_button_cap);

    struct ui_button *button = calloc(1, sizeof(*button) + len);
    *button = (struct ui_button) {
        .w = ui_widget_new(len * font->glyph_w, font->glyph_h),
        .font = font,
        .fg = rgba_white(),
        .state = ui_button_idle,
        .len = len,
        .str = (void *)(button + 1),
    };
    return button;
}

void ui_button_free(struct ui_button *button)
{
    free(button);
}

void ui_button_set(struct ui_button *button, const char *str, size_t len)
{
    assert((void *)(button + 1) == (void *)button->str);
    assert(len <= button->len);
    memcpy(button + 1, str, len);
}

enum ui_ret ui_button_event(struct ui_button *button, const SDL_Event *ev)
{
    struct SDL_Rect rect = ui_widget_rect(&button->w);

    switch (ev->type) {

    case SDL_MOUSEMOTION: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) {
            button->state = ui_button_idle;
            return ui_nil;
        }
        button->state = ui_button_hover;
        return ui_nil;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) return ui_nil;
        button->state = ui_button_pressed;
        return ui_consume;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_Point point = core.cursor.point;
        button->state = sdl_rect_contains(&rect, &point) ?
            ui_button_hover : ui_button_idle;
        return ui_nil;
    }

    default: { return ui_nil; }
    }
}

void ui_button_render(
        struct ui_button *button, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_layout_add(layout, &button->w);

    switch (button->state) {
    case ui_button_idle: { rgba_render(rgba_gray(0x22), renderer); break; }
    case ui_button_hover: { rgba_render(rgba_gray(0x33), renderer); break; }
    case ui_button_pressed: { rgba_render(rgba_gray(0x11), renderer); break; }
    default: { assert(false); }
    }

    SDL_Rect rect = ui_widget_rect(&button->w);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    SDL_Point point = {
        .x = button->w.pos.x + button->pad.w,
        .y = button->w.pos.y + button->pad.h
    };
    font_render(button->font, renderer, point, button->fg, button->str, button->len);
}
