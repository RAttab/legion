/* ux_pills.c
   Rémi Attab (remi.attab@gmail.com), 12 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

static void ux_pills_free(void *);
static void ux_pills_update(void *);
static void ux_pills_event(void *);
static void ux_pills_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// pills
// -----------------------------------------------------------------------------

enum ux_pills_sort
{
    ux_pills_sort_nil = 0,
    ux_pills_sort_source,
    ux_pills_sort_cargo,
};

struct ux_pills_data
{
    struct coord source;
    struct cargo cargo;
};

struct ux_pills_row
{
    struct ui_link source_link;
    struct ui_label cargo_item;
    struct ui_label cargo_count;
};

struct ux_pills
{
    struct coord star;

    size_t len;
    enum ux_pills_sort sort;
    struct ux_pills_data *data;

    struct ui_panel *panel;
    struct ui_scroll scroll;
    struct ui_button source_title, cargo_title;
    struct ux_pills_row *rows;
};


void ux_pills_alloc(struct ux_view_state *state)
{
    struct dim cell = engine_cell();
    unit width = (symbol_cap + item_str_len + 1 + 3 + 2) * cell.w;

    struct ux_pills *ux = mem_alloc_t(ux);
    *ux = (struct ux_pills) {
        .star = coord_nil(),
        .len = 0,
        .sort = ux_pills_sort_nil,

        .panel = ui_panel_title(make_dim(width, ui_layout_inf), ui_str_c("pills")),
        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), cell),
        .source_title = ui_button_new_s(&ui_st.button.list.close, ui_str_c("source")),
        .cargo_title = ui_button_new_s(&ui_st.button.list.close, ui_str_c("cargo")),
    };

    ux->source_title.w.w = symbol_cap * cell.w;
    ux->cargo_title.w.w = ui_layout_inf;

    ux->rows = mem_array_alloc_t(*ux->rows, pills_cap);
    ux->data = mem_array_alloc_t(*ux->data, pills_cap);

    for (size_t i = 0; i < pills_cap; ++i) {
        ux->rows[i] = (struct ux_pills_row) {
            .source_link = ui_link_new(ui_str_v(symbol_cap)),
            .cargo_item = ui_label_new(ui_str_v(item_str_len)),
            .cargo_count = ui_label_new(ui_str_v(3)),
        };
    }

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_pills,
        .parent = ux_view_star,
        .slots = ux_slot_right_sub,
        .panel = ux->panel,
        .fn = {
            .free = ux_pills_free,
            .update = ux_pills_update,
            .event = ux_pills_event,
            .render = ux_pills_render,
        },
    };
}

static void ux_pills_free(void *state)
{
    struct ux_pills *ux = state;

    ui_panel_free(ux->panel);
    ui_scroll_free(&ux->scroll);
    ui_button_free(&ux->source_title);
    ui_button_free(&ux->cargo_title);

    for (size_t i = 0; i < pills_cap; ++i) {
        struct ux_pills_row *row = ux->rows + i;
        ui_link_free(&row->source_link);
        ui_label_free(&row->cargo_item);
        ui_label_free(&row->cargo_count);
    }

    mem_free(ux->rows);
    mem_free(ux->data);
    mem_free(ux);
}

void ux_pills_show(struct coord star)
{
    struct ux_pills *ux = ux_state(ux_view_pills);

    ux->star = star;
    if (coord_is_nil(ux->star)) { ux_hide(ux_view_pills); return; }

    ux_pills_update(ux);
    ux_show(ux_view_pills);
}

static void ux_pills_sort(struct ux_pills *ux)
{
    int source_cmpv(const void *_lhs, const void *_rhs) {
        const struct ux_pills_data *lhs = _lhs;
        const struct ux_pills_data *rhs = _rhs;
        return coord_cmp(lhs->source, rhs->source);
    }

    int cargo_cmpv(const void *_lhs, const void *_rhs) {
        const struct ux_pills_data *lhs = _lhs;
        const struct ux_pills_data *rhs = _rhs;
        return cargo_cmp(lhs->cargo, rhs->cargo);
    }

    if (ux->sort == ux_pills_sort_source)
        qsort(ux->data, ux->len, sizeof(ux->data[0]), source_cmpv);
    else if (ux->sort == ux_pills_sort_cargo)
        qsort(ux->data, ux->len, sizeof(ux->data[0]), cargo_cmpv);

    for (size_t i = 0; i < ux->len; ++i) {
        struct ux_pills_data *data = ux->data + i;
        struct ux_pills_row *row = ux->rows + i;

        ui_str_set_coord_name(&row->source_link.str, data->source);
        ui_str_set_item(&row->cargo_item.str, data->cargo.item);
        ui_str_set_u64(&row->cargo_count.str, data->cargo.count);
    }
}

static void ux_pills_update(void *state)
{
    struct ux_pills *ux = state;

    struct chunk *chunk = proxy_chunk(ux->star);
    if (!chunk) { ux_pills_show(coord_nil()); return; }

    struct pills *pills = chunk_pills(chunk);

    ux->len = 0;
    size_t index = 0;
    struct pills_ret ret = {0};
    while ((ret = pills_next(pills, &index)).ok) {
        ux->data[ux->len] = (struct ux_pills_data) {
            .source = ret.coord,
            .cargo = ret.cargo,
        };
        ux->len++;
        index++;
    }

    ui_scroll_update_rows(&ux->scroll, ux->len);
    ux_pills_sort(ux);
}

static void ux_pills_event(void *state)
{
    struct ux_pills *ux = state;

    if (ui_button_event(&ux->source_title)) {
        if (ux->sort == ux_pills_sort_source) {
            ux->sort = ux_pills_sort_nil;
            ux->source_title.s = ui_st.button.list.close;
        }
        else {
            ux->sort = ux_pills_sort_source;
            ux->source_title.s = ui_st.button.list.open;
            ux->cargo_title.s = ui_st.button.list.close;
            ux_pills_sort(ux);
        }
    }

    if (ui_button_event(&ux->cargo_title)) {
        if (ux->sort == ux_pills_sort_cargo) {
            ux->sort = ux_pills_sort_nil;
            ux->cargo_title.s = ui_st.button.list.close;
        }
        else {
            ux->sort = ux_pills_sort_cargo;
            ux->cargo_title.s = ui_st.button.list.open;
            ux->source_title.s = ui_st.button.list.close;
            ux_pills_sort(ux);
        }
    }

    ui_scroll_event(&ux->scroll);

    size_t first = ui_scroll_first_row(&ux->scroll);
    size_t last = ui_scroll_last_row(&ux->scroll);
    for (size_t i = first; i < last; ++i) {
        if (ui_link_event(&ux->rows[i].source_link))
            ux_star_show(ux->data[i].source);
    }
}

static void ux_pills_render(void *state, struct ui_layout *layout)
{
    struct ux_pills *ux = state;

    ui_button_render(&ux->source_title, layout);
    ui_button_render(&ux->cargo_title, layout);
    ui_layout_next_row(layout);

    struct ui_layout inner = ui_scroll_render(&ux->scroll, layout);
    size_t first = ui_scroll_first_row(&ux->scroll);
    size_t last = ui_scroll_last_row(&ux->scroll);

    for (size_t i = first; i < last; ++i) {
        struct ux_pills_row *row = ux->rows + i;

        ui_link_render(&row->source_link, &inner);
        ui_label_render(&row->cargo_item, &inner);
        ui_layout_sep_col(&inner);
        ui_label_render(&row->cargo_count, &inner);
        ui_layout_next_row(&inner);
    }
}
