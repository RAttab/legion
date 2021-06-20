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
