/* button.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/render.h"
#include "render/font.h"
#include "utils/sdl.h"


// -----------------------------------------------------------------------------
// button
// -----------------------------------------------------------------------------

struct ui_button ui_button_new_s(const struct ui_button_style *s, struct ui_str str)
{
    return (struct ui_button) {
        .w = ui_widget_new(
                ui_st.font.dim.w * ui_str_len(&str) + s->margin.w*2,
                ui_st.font.dim.h + s->margin.h*2),
        .s = *s,
        .str = str,

        .disabled = false,
        .state = ui_button_idle,
    };
}

struct ui_button ui_button_new(struct ui_str str)
{
    return ui_button_new_s(&ui_st.button.base, str);
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
        SDL_Point point = render.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) {
            button->state = ui_button_idle;
            return ui_nil;
        }
        button->state = ui_button_hover;
        return ui_nil;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point point = render.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) return ui_nil;
        if (!button->disabled) button->state = ui_button_pressed;
        return button->disabled ? ui_consume : ui_action;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_Point point = render.cursor.point;
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

    const struct font *font = NULL;
    struct rgba fg = {0}, bg = {0};

    if (button->disabled) {
        font = button->s.disabled.font;
        fg = button->s.disabled.fg;
        bg = button->s.disabled.bg;
    }
    else {
        switch (button->state)
        {
        case ui_button_idle: {
            font = button->s.idle.font;
            fg = button->s.idle.fg;
            bg = button->s.idle.bg;
            break;
        }
        case ui_button_hover: {
            font = button->s.hover.font;
            fg = button->s.hover.fg;
            bg = button->s.hover.bg;
            break;
        }
        case ui_button_pressed: {
            font = button->s.pressed.font;
            fg = button->s.pressed.fg;
            bg = button->s.pressed.bg;
            break;
        }
        default: { assert(false); }
        }
    }

    rgba_render(bg, renderer);
    SDL_Rect rect = ui_widget_rect(&button->w);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    SDL_Point point = {
        .x = button->w.pos.x + button->s.margin.w,
        .y = button->w.pos.y + button->s.margin.h
    };
    font_render(font, renderer, point, fg, button->str.str, button->str.len);
}
