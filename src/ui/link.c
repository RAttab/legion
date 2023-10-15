/* link.c
   Rémi Attab (remi.attab@gmail.com), 13 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_link_style_default(struct ui_style *s)
{
    s->link = (struct ui_link_style) {
        .font = font_base,
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
        .w = make_ui_widget(engine_dim(ui_str_len(&str), 1)),
        .s = *s,
        .str = str,

        .disabled = false,
    };
}

void ui_link_free(struct ui_link *link)
{
    ui_str_free(&link->str);
}

bool ui_link_event(struct ui_link *link)
{
    bool ret = false;
    if (link->disabled) return ret;

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;
        if (ev->state != ev_state_up) continue;
        if (!ev_mouse_in(link->w)) continue;
        ev_consume_button(ev);
        ret = true;
    }

    return ret;
}

void ui_link_render(struct ui_link *link, struct ui_layout *layout)
{
    ui_layout_add(layout, &link->w);

    struct rgba fg = {0}, bg = {0};

    bool in = ev_mouse_in(link->w);
    bool down = ev_button_down(ev_button_left);

    if (link->disabled) {
        fg = link->s.disabled.fg;
        bg = link->s.disabled.bg;
    }
    else if (in && down) {
        fg = link->s.pressed.fg;
        bg = link->s.pressed.bg;
    }
    else if (in) {
        fg = link->s.hover.fg;
        bg = link->s.hover.bg;
    }
    else {
        fg = link->s.idle.fg;
        bg = link->s.idle.bg;
    }

    const render_layer layer = render_layer_push(1);

    render_font_bg(
            layer, link->s.font,
            fg, bg, rect_pos(link->w),
            link->str.str, link->str.len);

    render_layer_pop();
}
