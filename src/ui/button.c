/* button.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "button.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_button_style_default(struct ui_style *s)
{
    struct dim cell = engine_cell();
    s->button.base = (struct ui_button_style) {
        .idle = {
            .font = font_base,
            .fg = s->rgba.fg,
            .bg = rgba_gray(0x22)
        },
        .hover = {
            .font = font_base,
            .fg = s->rgba.fg,
            .bg = rgba_gray(0x33)
        },
        .pressed = {
            .font = font_base,
            .fg = s->rgba.fg,
            .bg = rgba_gray(0x11)
        },
        .disabled = {
            .font = font_base,
            .fg = s->rgba.disabled,
            .bg = rgba_gray(0x22)
        },

        .height = cell.h + (s->pad.box.h * 2),
        .margin = s->pad.box,
        .align = ui_align_left,
    };

    s->button.line = s->button.base;
    s->button.line.margin.h = 0;
    s->button.line.height = cell.h;

    s->button.list.close = s->button.base;
    s->button.list.close.idle.bg = ui_st.rgba.bg;

    s->button.list.open = s->button.base;
    s->button.list.open.idle.font = font_bold;
    s->button.list.open.hover.font = font_bold;
    s->button.list.open.pressed.font = font_bold;
    s->button.list.open.disabled.font = font_bold;
}


// -----------------------------------------------------------------------------
// button
// -----------------------------------------------------------------------------

struct ui_button ui_button_new_s(const struct ui_button_style *s, struct ui_str str)
{
    return (struct ui_button) {
        .w = make_ui_widget(engine_dim_margin(ui_str_len(&str), 1, s->margin)),
        .s = *s,
        .str = str,

        .disabled = false,
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

bool ui_button_event(struct ui_button *button)
{
    bool ret = false;
    if (button->disabled) return ret;

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;
        if (!ev_mouse_in(button->w)) continue;

        if (ev->state == ev_state_up) {
            ev_consume_button(ev);
            ret = true;
        }
    }

    return ret;
}

void ui_button_render(struct ui_button *button, struct ui_layout *layout)
{
    ui_layout_add(layout, &button->w);

    enum render_font font = font_nil;
    struct rgba fg = {0}, bg = {0};

    bool in = ev_mouse_in(button->w);
    bool down = ev_button_down(ev_button_left);

    if (button->disabled) {
        font = button->s.disabled.font;
        fg = button->s.disabled.fg;
        bg = button->s.disabled.bg;
    }
    else if (in && down) {
        font = button->s.pressed.font;
        fg = button->s.pressed.fg;
        bg = button->s.pressed.bg;
    }
    else if (in) {
        font = button->s.hover.font;
        fg = button->s.hover.fg;
        bg = button->s.hover.bg;
    }
    else {
        font = button->s.idle.font;
        fg = button->s.idle.fg;
        bg = button->s.idle.bg;
    }

    struct pos pos = {
        .x = button->w.x,
        .y = button->w.y + button->s.margin.h,
    };
    struct dim cell = engine_cell();

    switch (button->s.align)
    {

    case ui_align_left: {
        pos.x += button->s.margin.w;
        break;
    }

    case ui_align_center: {
        pos.x += (button->w.w / 2);
        pos.x -= (button->str.len * cell.w) / 2;
        break;
    }

    case ui_align_right: {
        pos.x += button->w.w;
        pos.x -= button->s.margin.w;
        pos.x -= button->str.len * cell.w;
        break;
    }

    default: { assert(false); }
    }

    const render_layer layer_bg = render_layer_push(2);
    const render_layer layer_fg = layer_bg + 1;

    render_rect_fill(layer_bg, bg, button->w);
    render_font(layer_fg, font, fg, pos, button->str.str, button->str.len);

    render_layer_pop();
}
