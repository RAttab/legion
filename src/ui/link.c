/* link.c
   Rémi Attab (remi.attab@gmail.com), 13 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/font.h"
#include "render/core.h"
#include "utils/sdl.h"


// -----------------------------------------------------------------------------
// link
// -----------------------------------------------------------------------------

struct ui_link ui_link_new(struct font *font, struct ui_str str)
{
    return (struct ui_link) {
        .w = ui_widget_new(ui_str_len(&str) * font->glyph_w, font->glyph_h),
        .str = str,
        .font = font,
        .fg = rgba_white(),
    };
}

void ui_link_free(struct ui_link *link)
{
    ui_str_free(&link->str);
}

enum ui_ret ui_link_event(struct ui_link *link, const SDL_Event *ev)
{
    struct SDL_Rect rect = ui_widget_rect(&link->w);

    switch (ev->type) {

    case SDL_MOUSEMOTION: {
        SDL_Point point = core.cursor.point;
        link->state = sdl_rect_contains(&rect, &point) ? ui_link_hover : ui_link_idle;
        return ui_nil;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) return ui_nil;
        link->state = ui_link_pressed;
        return ui_consume;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_Point point = core.cursor.point;
        link->state = sdl_rect_contains(&rect, &point) ? ui_link_hover : ui_link_idle;
        return ui_nil;
    }

    default: { return ui_nil; }
    }
}

void ui_link_render(
        struct ui_link *link, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_layout_add(layout, &link->w);

    switch (link->state) {
    case ui_link_idle: { rgba_render(rgba_nil(), renderer); break; }
    case ui_link_hover: { rgba_render(rgba_gray(0x44), renderer); break; }
    case ui_link_pressed: { rgba_render(rgba_gray(0x88), renderer); break; }
    default: { assert(false); }
    }

    SDL_Rect rect = ui_widget_rect(&link->w);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    SDL_Point point = pos_as_point(link->w.pos);
    font_render(link->font, renderer, point, link->fg, link->str.str, link->str.len);
}
