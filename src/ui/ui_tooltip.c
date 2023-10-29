/* ui_tooltip.c
   Rémi Attab (remi.attab@gmail.com), 23 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_tooltip_style_default(struct ui_style *s)
{
    s->tooltip = (struct ui_tooltip_style) {
        .font = font_base,
        .fg = s->rgba.fg,
        .bg = s->rgba.box.bg,
        .border = s->rgba.box.border,
        .pad = s->pad.box,
    };
}


// -----------------------------------------------------------------------------
// tooltip
// -----------------------------------------------------------------------------

struct ui_tooltip
{
    struct ui_tooltip_style s;
    struct ui_str str;
    struct rowcol rc;
    struct rect rect;
} ui_tooltip = {0};


void ui_tooltip_init(void)
{
    ui_tooltip.s = ui_st.tooltip;
}

void ui_tooltip_free(void)
{
    ui_str_free(&ui_tooltip.str);

}

void ui_tooltip_set(struct rect rect, struct ui_str str)
{
    ui_str_free(&ui_tooltip.str);

    ui_tooltip.str = str;
    ui_tooltip.rect = rect;

    uint32_t cols = 0;
    struct rowcol rc = { .row = 1, .col = 0 };
    for (size_t i = 0; i < str.len; ++i) {
        if (unlikely(str.str[i] == '\n')) { rc.row++; cols = 0; continue; }
        rc.col = legion_max(rc.col, ++cols);
    }

    ui_tooltip.rc = rc;
}

void ui_tooltip_unset(void)
{
    ui_tooltip.rect = rect_nil();
}

void ui_tooltip_render(void)
{
    if (rect_is_nil(ui_tooltip.rect)) return;

    struct pos cursor = ev_mouse_pos();
    if (!rect_contains(ui_tooltip.rect, cursor)) return;

    struct rect area = engine_area();
    unit cursor_w = render_cursor_size();
    struct dim box = engine_dim_margin(
            ui_tooltip.rc.col, ui_tooltip.rc.row, ui_tooltip.s.pad);

    bool right = cursor.x + cursor_w + box.w <= area.x + area.w;
    bool left = cursor.x - box.w >= area.x;

    struct rect rect = {
        .x = right || !left ? cursor.x + cursor_w : cursor.x - box.w,
        .y = cursor.y, .w = box.w, .h = box.h
    };

    const render_layer layer_bg = render_layer_push(2);
    const render_layer layer_fg = layer_bg + 1;

    render_rect_fill(layer_bg, ui_tooltip.s.bg, rect);
    render_rect_line(layer_fg, ui_tooltip.s.border, rect);
    render_font(
            layer_fg, ui_tooltip.s.font, ui_tooltip.s.fg,
            make_pos(rect.x + ui_tooltip.s.pad.w, rect.y + ui_tooltip.s.pad.h),
            ui_tooltip.str.str, ui_tooltip.str.len);

    render_layer_pop();
}
