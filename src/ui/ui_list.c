/* ui_list.c
   Rémi Attab (remi.attab@gmail.com), 15 May 2022
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_list_style_default(struct ui_style *s)
{
    s->list = (struct ui_list_style) {
        .idle = {
            .font = font_base,
            .fg = s->rgba.fg,
            .bg = s->rgba.bg,
        },

        .hover = {
            .font = font_base,
            .fg = s->rgba.fg,
            .bg = s->rgba.list.hover,
        },

        .selected = {
            .font = font_bold,
            .fg = s->rgba.fg,
            .bg = s->rgba.list.selected,
        },
    };
}



// -----------------------------------------------------------------------------
// list
// -----------------------------------------------------------------------------

struct ui_list ui_list_new(struct dim dim, size_t chars)
{
    const struct ui_list_style *s = &ui_st.list;

    return (struct ui_list) {
        .w = make_ui_widget(dim),
        .s = *s,

        .scroll = ui_scroll_new(dim, engine_cell()),
        .str = ui_str_v(chars),

        .len = 0,
        .cap = 0,
        .entries = NULL,

        .selected = 0,
    };
}

void ui_list_free(struct ui_list *list)
{
    ui_scroll_free(&list->scroll);

    ui_str_free(&list->str);
    for (size_t i = 0; i < list->cap; ++i)
        ui_str_free(&list->entries[i].str);
    free(list->entries);
}


void ui_list_clear(struct ui_list *list)
{
    list->selected = 0;
}

bool ui_list_select(struct ui_list *list, uint64_t user)
{
    for (size_t i = 0; i < list->len; ++i) {
        if (list->entries[i].user != user) continue;
        list->selected = user;
        return true;
    }

    list->selected = 0;
    return false;
}

void ui_list_reset(struct ui_list *list)
{
    list->len = 0;
}

struct ui_str *ui_list_add(struct ui_list *list, uint64_t user)
{
    assert(user);

    if (list->len == list->cap) {
        size_t old = list->cap;
        list->cap = list->cap ? list->cap * 2 : 8;
        list->entries = realloc_zero(list->entries, old, list->cap, sizeof(*list->entries));
    }

    struct ui_list_entry *entry = list->entries + list->len;
    list->len++;

    entry->user = user;

    if (!entry->str.cap)
        entry->str = ui_str_clone(&list->str);

    return &entry->str;
}


uint64_t ui_list_event(struct ui_list *list)
{
    ui_scroll_event(&list->scroll);

    uint64_t ret = 0;

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;

        struct pos cursor = ev_mouse_pos();
        if (!rect_contains(list->w, cursor)) continue;
        ev_consume_button(ev);

        size_t row = (cursor.y - list->w.y) / engine_cell().h;
        row += ui_scroll_first_row(&list->scroll);
        if (row >= list->len) continue;

        ret = list->selected = list->entries[row].user;
    }

    return ret;
}

void ui_list_render(struct ui_list *list, struct ui_layout *layout)
{
    ui_scroll_update_rows(&list->scroll, list->len);

    struct ui_layout inner = ui_scroll_render(&list->scroll, layout);
    if (ui_layout_is_nil(&inner)) return;
    list->w = list->scroll.w;

    const render_layer layer_bg = render_layer_push(2);
    const render_layer layer_fg = layer_bg + 1;

    struct dim cell = engine_cell();
    struct pos pos = inner.base.pos;

    size_t last = ui_scroll_last_row(&list->scroll);
    size_t first = ui_scroll_first_row(&list->scroll);

    for (size_t i = first; i < last; ++i) {
        const struct ui_list_entry *entry = list->entries + i;

        struct rect rect = {
            .x = pos.x, .y = pos.y,
            .w = inner.base.dim.w, .h = cell.h,
        };

        enum render_font font = font_nil;
        struct rgba fg = {0}, bg = {0};

        bool selected = entry->user == list->selected;
        bool hover = ev_mouse_in(rect);

        if (hover) {
            font = selected ? list->s.selected.font : list->s.hover.font;
            fg = list->s.hover.fg;
            bg = list->s.hover.bg;
        }
        else if (selected) {
            font = list->s.selected.font;
            fg = list->s.selected.fg;
            bg = list->s.selected.bg;
        }
        else {
            font = list->s.idle.font;
            fg = list->s.idle.fg;
            bg = list->s.idle.bg;
        }

        render_rect_fill(layer_bg, bg, rect);
        render_font(layer_fg, font, fg, pos, entry->str.str, entry->str.len);
        pos.y += cell.h;
    }

    render_layer_pop();
}
