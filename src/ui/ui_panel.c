/* ui_panel.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_panel_style_default(struct ui_style *s)
{
    struct dim cell = engine_cell();
    s->panel = (struct ui_panel_style) {
        .margin = make_dim(cell.h / 4, cell.h / 4),

        .bg = rgba_gray_a(0x11, 0xEE),
        .border = s->rgba.box.border,

        .head = {
            .font = font_base,
            .fg = rgba_gray(0xAA),
            .bg = rgba_gray(0x11),
        },

        .focused = {
            .font = font_bold,
            .fg = s->rgba.fg,
            .bg = rgba_gray(0x22),
        },
    };
}


// -----------------------------------------------------------------------------
// panel
// -----------------------------------------------------------------------------

static struct ui_panel *ui_panel_curr = NULL;
struct ui_panel *ui_panel_current(void) { return ui_panel_curr; }

static struct ui_panel *ui_panel_new(struct dim dim)
{
    struct ui_panel_style *s = &ui_st.panel;

    struct ui_panel *panel = mem_alloc_t(panel);
    *panel = (struct ui_panel) {
        .w = make_ui_widget(dim),
        .s = *s,
        .visible = true,
    };

    if (panel->w.w != ui_layout_inf)
        panel->w.w += panel->s.margin.w * 2;
    if (panel->w.h != ui_layout_inf)
        panel->w.h += panel->s.margin.h * 2;

    ui_panel_curr = panel;
    return panel;
}

struct ui_panel *ui_panel_menu(struct dim dim)
{
    struct ui_panel *panel = ui_panel_new(dim);
    panel->menu = true;
    return panel;
}

struct ui_panel *ui_panel_title(struct dim dim, struct ui_str str)
{
    struct ui_panel *panel = ui_panel_new(dim);
    panel->title = ui_label_new(str);
    panel->close = ui_button_new(ui_str_c("X"));
    return panel;
}

void ui_panel_free(struct ui_panel *panel)
{
    if (!panel->menu) {
        ui_label_free(&panel->title);
        ui_button_free(&panel->close);
    }
    mem_free(panel);
}

void ui_panel_resize(struct ui_panel *panel, struct dim dim)
{
    if (dim.w != ui_layout_inf)
        panel->w.w = dim.w + (panel->s.margin.w * 2);
    if (dim.h != ui_layout_inf)
        panel->w.h = dim.h + (panel->s.margin.h * 2);
}


void ui_panel_show(struct ui_panel *panel)
{
    panel->visible = true;
    ui_focus_panel_acquire(panel);
}

void ui_panel_hide(struct ui_panel *panel)
{
    panel->visible = false;
    ui_focus_panel_release(panel);
}


enum ui_panel_ev ui_panel_event(struct ui_panel *panel)
{
    enum ui_panel_ev ret = ui_panel_ev_nil;
    if (!panel->visible) return ui_panel_ev_skip;

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;
        if (ev->state != ev_state_down) continue;

        if (!panel->menu) {
            struct rect title = panel->w;
            title.h = panel->close.w.h + panel->s.margin.h;
            if (ev_mouse_in(title)) ev_consume_button(ev);
        }

        ev_mouse_in(panel->w) ?
            ui_focus_panel_acquire(panel) :
            ui_focus_panel_release(panel);
    }

    if (ui_button_event(&panel->close)) {
        ret = ui_panel_ev_close;
        ui_panel_hide(panel);
    }

    return ret;
}

void ui_panel_event_consume(struct ui_panel *panel)
{
    if (!panel->visible) return;

    if (ui_focus_panel() == panel)
        for (auto ev = ev_next_key(nullptr); ev; ev = ev_next_key(ev))
            ev_consume_key(ev);

    if (ev_mouse_in(panel->w))
        for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev))
            ev_consume_button(ev);
}

struct ui_layout ui_panel_render(struct ui_panel *panel, struct ui_layout *layout)
{
    if (!panel->visible) return (struct ui_layout) {0};

    ui_layout_add(layout, &panel->w);
    const bool focused = ui_focus_panel() == panel;
    const render_layer layer_bg = render_layer_push(3);
    const render_layer layer_title = layer_bg + 1;
    const render_layer layer_border = layer_bg + 2;

    render_rect_fill(layer_bg, panel->s.bg, panel->w);

    if (!panel->menu) {
        struct rgba title_bg = focused ? panel->s.focused.bg : panel->s.head.bg;
        struct rect title = panel->w;
        title.h = panel->close.w.h + panel->s.margin.h;

        render_rect_fill(layer_title, title_bg, title);
        render_rect_line(layer_border, panel->s.border, panel->w);
    }

    struct ui_layout inner = ui_layout_new(
            make_pos(
                    panel->w.x + panel->s.margin.w,
                    panel->w.y + panel->s.margin.h),
            make_dim(
                    panel->w.w - (panel->s.margin.w * 2),
                    panel->w.h - (panel->s.margin.h * 2)));

    if (panel->menu) return inner;

    if (focused) {
        panel->title.s.font = panel->s.focused.font;
        panel->title.s.fg = panel->s.focused.fg;
    }
    else {
        panel->title.s.font = panel->s.head.font;
        panel->title.s.fg = panel->s.head.fg;
    }
    ui_label_render(&panel->title, &inner);

    ui_layout_dir(&inner, ui_layout_right_left);
    ui_button_render(&panel->close, &inner);

    ui_layout_next_row(&inner);
    ui_layout_sep_y(&inner, 4);
    ui_layout_dir(&inner, ui_layout_left_right);

    return inner;
}
