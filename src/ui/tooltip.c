/* tooltip.c
   Rémi Attab (remi.attab@gmail.com), 23 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/font.h"
#include "render/render.h"
#include "utils/sdl.h"


// -----------------------------------------------------------------------------
// tooltip
// -----------------------------------------------------------------------------

struct ui_tooltip ui_tooltip_new(
        struct font *font, struct ui_str str, SDL_Rect rect)
{
    struct dim pad = make_dim(6, 2);
    return (struct ui_tooltip) {
        .w = ui_widget_new(
                font->glyph_w * ui_str_len(&str) + pad.w*2,
                font->glyph_h + pad.h*2),
        .str = str,

        .font = font,
        .fg = rgba_white(),
        .pad = pad,

        .rect = rect,
        .visible = false,
    };
}

void ui_tooltip_free(struct ui_tooltip *tooltip)
{
    ui_str_free(&tooltip->str);
}

enum ui_ret ui_tooltip_event(struct ui_tooltip *tooltip, const SDL_Event *ev)
{
    switch (ev->type) {

    case SDL_MOUSEMOTION: {
        SDL_Point cursor = render.cursor.point;
        tooltip->w.pos = make_pos(cursor.x + render.cursor.size, cursor.y);

        if (!tooltip->rect.w && !tooltip->rect.h)
            tooltip->visible = sdl_rect_contains(&tooltip->rect, &cursor);

        return ui_nil;
    }

    default: { return ui_nil; }
    }
}

void ui_tooltip_render(struct ui_tooltip *tooltip, SDL_Renderer *renderer)
{
    if (!tooltip->visible) return;

    SDL_Rect rect = ui_widget_rect(&tooltip->w);

    rgba_render(rgba_black(), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    rgba_render(rgba_gray(0x33), renderer);
    sdl_err(SDL_RenderDrawRect(renderer, &rect));

    SDL_Point point = {
        .x = tooltip->w.pos.x + tooltip->pad.w,
        .y = tooltip->w.pos.y + tooltip->pad.h
    };
    font_render(tooltip->font, renderer, point, tooltip->fg, tooltip->str.str, tooltip->str.len);
}
