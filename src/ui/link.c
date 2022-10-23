/* link.c
   Rémi Attab (remi.attab@gmail.com), 13 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "link.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_link_style_default(struct ui_style *s)
{
    s->link = (struct ui_link_style) {
        .font = s->font.base,
#define make_from(src) { .fg = (src).fg, .bg = (src).bg }
        .idle =     make_from(s->rgba.link.idle),
        .hover =    make_from(s->rgba.link.hover),
        .pressed =  make_from(s->rgba.link.pressed),
#undef make_from
        .disabled = { .fg = s->rgba.disabled, .bg = s->rgba.bg },
    };
}


// -----------------------------------------------------------------------------
// link
// -----------------------------------------------------------------------------

struct ui_link ui_link_new(struct ui_str str)
{
    const struct ui_link_style *s = &ui_st.link;

    return (struct ui_link) {
        .w = ui_widget_new(ui_str_len(&str) * s->font->glyph_w, s->font->glyph_h),
        .s = *s,
        .str = str,

        .state = ui_link_idle,
        .disabled = false,
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
        SDL_Point point = render.cursor.point;
        link->state = sdl_rect_contains(&rect, &point) ? ui_link_hover : ui_link_idle;
        return ui_nil;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point point = render.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) return ui_nil;
        link->state = ui_link_pressed;
        return ui_consume;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_Point point = render.cursor.point;
        if (!sdl_rect_contains(&rect, &point)) {
            link->state = ui_link_idle;
            return ui_nil;
        }

        link->state = ui_link_hover;
        return ui_action;
    }

    default: { return ui_nil; }
    }
}

void ui_link_render(
        struct ui_link *link, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_layout_add(layout, &link->w);

    struct rgba fg = {0}, bg = {0};

    if (link->disabled) {
        fg = link->s.disabled.fg;
        bg = link->s.disabled.bg;
    }
    else {
        switch (link->state) {
        case ui_link_idle: { fg = link->s.idle.fg; bg = link->s.idle.bg; break; }
        case ui_link_hover: { fg = link->s.hover.fg; bg = link->s.hover.bg; break; }
        case ui_link_pressed: { fg = link->s.pressed.fg; bg = link->s.pressed.bg; break; }
        default: { assert(false); }
        }
    }

    SDL_Point point = pos_as_point(link->w.pos);
    font_render_bg(link->s.font, renderer, point, fg, bg, link->str.str, link->str.len);
}
