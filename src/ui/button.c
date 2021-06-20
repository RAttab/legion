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

struct ui_button ui_button_new(struct font *font, struct ui_str str)
{
    struct dim pad = make_dim(6, 2);
    return (struct ui_button) {
        .w = ui_widget_new(
                font->glyph_w * ui_str_len(&str) + pad.w*2,
                font->glyph_h + pad.h*2),
        .str = str,
        .font = font,
        .pad = pad,
        .state = ui_button_idle,
    };
}

void ui_button_free(struct ui_button *button)
{
    ui_str_free(&button->str);
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
    font_render(button->font, renderer, point, rgba_white(), button->str.str, button->str.len);
}
