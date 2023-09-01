/* tooltip.c
   Rémi Attab (remi.attab@gmail.com), 23 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "tooltip.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_tooltip_style_default(struct ui_style *s)
{
    s->tooltip = (struct ui_tooltip_style) {
        .font = s->font.base,
        .fg = s->rgba.fg,
        .bg = s->rgba.box.bg,
        .border = s->rgba.box.border,
        .pad = s->pad.box,
    };
}


// -----------------------------------------------------------------------------
// tooltip
// -----------------------------------------------------------------------------

struct ui_tooltip ui_tooltip_new(struct ui_str str, SDL_Rect rect)
{
    const struct ui_tooltip_style *s = &ui_st.tooltip;

    return (struct ui_tooltip) {
        .w = ui_widget_new(
                s->font->glyph_w * ui_str_len(&str) + s->pad.w*2,
                s->font->glyph_h + s->pad.h*2),
        .s = *s,
        .str = str,

        .rect = rect,
        .disabled = true,
    };
}

void ui_tooltip_free(struct ui_tooltip *tooltip)
{
    ui_str_free(&tooltip->str);
}

void ui_tooltip_show(struct ui_tooltip *tooltip) { tooltip->disabled = false; }
void ui_tooltip_hide(struct ui_tooltip *tooltip) { tooltip->disabled = true; }

enum ui_ret ui_tooltip_event(struct ui_tooltip *tooltip, const SDL_Event *ev)
{
    switch (ev->type) {

    case SDL_MOUSEMOTION: {
        SDL_Point cursor = ui_cursor_point();
        tooltip->w.pos = make_pos(cursor.x + ui_cursor_size(), cursor.y);

        if (!tooltip->rect.w && !tooltip->rect.h)
            tooltip->disabled = !SDL_PointInRect(&cursor, &tooltip->rect);

        return ui_nil;
    }

    default: { return ui_nil; }
    }
}

void ui_tooltip_render(struct ui_tooltip *tooltip, SDL_Renderer *renderer)
{
    if (tooltip->disabled) return;

    SDL_Rect rect = {
        .x = tooltip->w.pos.x,
        .y = tooltip->w.pos.y,
        .w = tooltip->s.font->glyph_w * tooltip->str.len + tooltip->s.pad.w * 2,
        .h = tooltip->s.font->glyph_h                    + tooltip->s.pad.h * 2,
    };

    rgba_render(tooltip->s.bg, renderer);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    rgba_render(tooltip->s.border, renderer);
    sdl_err(SDL_RenderDrawRect(renderer, &rect));

    SDL_Point point = {
        .x = tooltip->w.pos.x + tooltip->s.pad.w,
        .y = tooltip->w.pos.y + tooltip->s.pad.h
    };
    font_render(
            tooltip->s.font,
            renderer,
            point,
            tooltip->s.fg,
            tooltip->str.str, tooltip->str.len);
}
