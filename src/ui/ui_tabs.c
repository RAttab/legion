/* ui_tabs.c
   Rémi Attab (remi.attab@gmail.com), 04 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_tabs_style_default(struct ui_style *s)
{
    s->tabs = (struct ui_tabs_style) {
        .margin = 4,
        .pad = s->pad.box,

        .font = font_base,
        .bold = font_bold,

        .fg = s->rgba.fg,
        .line = s->rgba.box.border,
        .hover = rgba_gray(0x33),
        .pressed = rgba_gray(0x22),
    };
}



// -----------------------------------------------------------------------------
// tabs
// -----------------------------------------------------------------------------

struct ui_tabs ui_tabs_new(size_t chars, bool close)
{
    const struct ui_tabs_style *s = &ui_st.tabs;

    struct dim cell = engine_cell();
    return (struct ui_tabs) {
        .w = make_ui_widget(make_dim(
                        ui_layout_inf,
                        cell.h + (s->pad.h * 2) + s->margin)),
        .s = *s,

        .str = ui_str_v(chars),
        .close = { .show = close },
    };
}

void ui_tabs_free(struct ui_tabs *ui)
{
    for (size_t i = 0; i < ui->cap; ++i) {
        struct ui_tab *tab = ui->list + i;
        if (tab->str.cap) ui_str_free(&tab->str);
    }

    ui_str_free(&ui->str);
    mem_free(ui->list);
}


void ui_tabs_clear(struct ui_tabs *ui)
{
    ui->select.user = 0;
    ui->select.update = false;
}

void ui_tabs_select(struct ui_tabs *ui, uint64_t user)
{
    if (ui->select.user == user) return;

    ui->select.user = user;
    ui->select.update = true;
}

uint64_t ui_tabs_selected(struct ui_tabs *ui)
{
    return ui->select.user;
}

uint64_t ui_tabs_closed(struct ui_tabs *ui)
{
    return legion_xchg(&ui->close.user, 0UL);
}

void ui_tabs_reset(struct ui_tabs *ui)
{
    ui->len = 0;
}

struct ui_str *ui_tabs_add_s(struct ui_tabs *ui, uint64_t user, struct rgba fg)
{
    assert(user);
    ui->update = true;

    if (ui->len == ui->cap) {
        size_t old = mem_array_len_grow(&ui->cap, 8);
        ui->list = mem_array_realloc_t(ui->list, old, ui->cap);
    }

    struct ui_tab *tab = ui->list + ui->len;
    ui->len++;

    tab->fg = fg;
    tab->user = user;

    if (!tab->str.cap) tab->str = ui_str_clone(&ui->str);
    return &tab->str;
}

struct ui_str *ui_tabs_add(struct ui_tabs *ui, uint64_t user)
{
    return ui_tabs_add_s(ui, user, ui->s.fg);
}

static void ui_tabs_update(struct ui_tabs *ui)
{
    if (!ui->update && !ui->select.update) return;
    struct dim cell = engine_cell();

    unit x = ui->w.x;
    unit w = ui->w.w;
    ui->first = legion_min(ui->first, ui->len - 1);

    {
        ui->left.w = ui->s.pad.w * 2 + 1 * cell.w;
        ui->left.x = x;
        ui->right.w = ui->left.w;
        ui->right.x = x + w - ui->right.w;

        x += ui->left.w;
        w -= ui->left.w + ui->right.w;
    }

    {
        unit sum = 0;
        for (size_t i = 0; i < ui->len; ++i) {
            struct ui_tab *tab = ui->list + i;
            tab->w =
                (tab->str.len * cell.w) +
                (ui->close.show ? 2 * cell.w : 0) +
                ui->s.pad.w * 2;
            sum += tab->w;
        }

        if (sum < w) ui->first = 0;
    }

    if (ui->select.update) {
        size_t ix = 0;
        while (ix < ui->len && ui->list[ix].user != ui->select.user) ++ix;
        assert(ui->select.user);
        assert(ix < ui->len);

        unit sum = 0;
        ui->first = legion_min(ui->first, ix);
        for (; ix > ui->first; --ix) {
            struct ui_tab *tab = ui->list + ix;
            if (sum + tab->w > w) break;
            sum += tab->w;
        }
        ui->first = ix;
        ui->select.update = false;
    }

    {
        unit sum = 0;
        for (size_t i = ui->first; i < ui->len; ++i) {
            struct ui_tab *tab = ui->list + i;
            if (sum + tab->w > w) break;
            sum += tab->w;
        }

        for (; ui->first; --ui->first) {
            struct ui_tab *prev = ui->list + (ui->first - 1);
            if (sum + prev->w > w) break;
            sum += prev->w;
        }

    }

    ui->left.show = ui->first > 0;
    ui->right.show = false;

    for (size_t i = 0; i < ui->len; ++i) {
        struct ui_tab *tab = ui->list + i;

        tab->hidden =
            i < ui->first ||
            ui->right.show ||
            (ui->right.show = tab->w > w);
        if (tab->hidden) continue;

        tab->x = x;
        x += tab->w;
        w -= tab->w;
    }

    ui->update = false;
}

enum ui_tabs_ev ui_tabs_event(struct ui_tabs *ui)
{
    enum ui_tabs_ev ret = ui_tabs_ev_nil;
    struct dim cell = engine_cell();

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;
        if (ev->state != ev_state_up) continue;

        if (rect_contains(ui->w, ev_mouse_pos()))
            ev_consume_button(ev);

        struct rect rect = {
            .x = 0, .y = ui->w.y,
            .w = 0, .h = ui->w.h,
        };

        rect.x = ui->left.x; rect.w = ui->left.w;
        if (ui->left.show && ev_mouse_in(rect)) {
            assert(ui->first > 0);
            ui->first--;
            ui->update = true;
            continue;
        }

        rect.x = ui->right.x; rect.w = ui->right.w;
        if (ui->right.show && ev_mouse_in(rect)) {
            assert(ui->first > 0);
            ui->first++;
            ui->update = true;
            continue;
        }

        for (size_t i = ui->first; i < ui->len; ++i) {
            struct ui_tab *tab = ui->list + i;
            if (tab->hidden) continue;

            rect.x = tab->x; rect.w = tab->w;
            if (!ev_mouse_in(rect)) continue;

            rect.w = cell.w;
            rect.x = (tab->x + tab->w) - (ui->s.pad.w + rect.w);
            if (!ui->close.show || !ev_mouse_in(rect)) {
                ui->select.user = tab->user;
                ret = ui_tabs_ev_select;
                break;
            }

            ui->close.user = tab->user;
            struct ui_tab swap = *tab;
            memmove(tab, tab + 1, (ui->len - (i + 1)) * sizeof(*tab));
            *(ui->list + (ui->len - 1)) = swap;
            ui->len--;

            if (ui->close.user == ui->select.user) {
                if (i < ui->len) ui->select.user = tab->user;
                else if (i > 0) ui->select.user = (tab - 1)->user;
                else ui->select.user = 0;
            }

            ret = ui_tabs_ev_close;
            ui->update = true;
            break;
        }
    }

    return ret;
}

void ui_tabs_render(struct ui_tabs *ui, struct ui_layout *layout)
{
    ui_layout_add(layout, &ui->w);
    ui_tabs_update(ui);

    struct dim cell = engine_cell();
    const unit h = ui->w.h - ui->s.margin;
    const unit y0 = ui->w.y;
    const unit y1 = y0 + h - 1;

    enum : render_layer
    {
        layer_bg = 0,
        layer_border,
        layer_fg,
        layer_len,
    };
    const render_layer l = render_layer_push(layer_len);

    { // left
        struct rect rect = { .x = ui->left.x, .y = y0, .w = ui->left.w, .h = h };

        if (ui->left.show) {
            if (ev_mouse_in(rect)) {
                struct rgba bg = ev_button_down(ev_button_left) ?
                    ui->s.pressed : ui->s.hover;
                render_rect_fill(l + layer_bg, bg, rect);
            }

            struct pos pos = { rect.x + ui->s.pad.w,rect.y + ui->s.pad.h };
            render_font(l + layer_fg, ui->s.font, ui->s.fg, pos, "<", 1);
        }

        render_line(l + layer_border, ui->s.line, (struct line) {
                    { ui->left.x, y1 },
                    { ui->left.x + ui->left.w, y1 }});
    }


    unit x_last = 0;

    for (size_t i = ui->first; i < ui->len; ++i) {
        struct ui_tab *tab = ui->list + i;
        if (tab->hidden) break;

        struct rect rect = { .x = tab->x, .y = y0, .w = tab->w, .h = h };
        if (ev_mouse_in(rect)) {
            struct rgba bg = ev_button_down(ev_button_left) ?
                ui->s.pressed : ui->s.hover;
            render_rect_fill(l + layer_bg, bg, rect);
        }

        enum render_font font = tab->user == ui->select.user ?
            ui->s.bold : ui->s.font;

        struct pos pos = { .x = rect.x + ui->s.pad.w, .y = rect.y + ui->s.pad.h };
        render_font(l + layer_fg, font, tab->fg, pos, tab->str.str, tab->str.len);

        if (ui->close.show) {
            pos.x = (rect.x + rect.w) - (ui->s.pad.w + cell.w);
            render_font(l + layer_fg, ui->s.font, ui->s.fg, pos, "x", 1);
        }

        const unit x0 = tab->x;
        const unit x1 = x0 + tab->w;
        x_last = x1;

        if (tab->user != ui->select.user) {
            render_rect_line(l + layer_border, ui->s.line, rect);
            continue;
        }

        render_line(l + layer_border, ui->s.line, (struct line) {{x0, y0}, {x0, y1}});
        render_line(l + layer_border, ui->s.line, (struct line) {{x0, y0}, {x1, y0}});
        render_line(l + layer_border, ui->s.line, (struct line) {{x1, y0}, {x1, y1}});
    }

    { // right
        struct rect rect = { .x = ui->right.x, .y = y0, .w = ui->right.w, .h = h };
        if (ui->left.show) {
            if (ev_mouse_in(rect)) {
                struct rgba bg = ev_button_down(ev_button_left) ?
                    ui->s.pressed : ui->s.hover;
                render_rect_fill(l + layer_bg, bg, rect);
            }

            struct pos pos = { .x = rect.x + ui->s.pad.w, .y = rect.y + ui->s.pad.h };
            render_font(l + layer_fg, ui->s.font, ui->s.fg, pos, ">", 1);
        }

        const unit x1 = ui->w.x + ui->w.w;
        render_line(l + layer_border, ui->s.line, (struct line) {
                    {x_last, y1}, {x1, y1}});
    }

    render_layer_pop();
}
