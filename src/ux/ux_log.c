/* ux_log.c
   Rémi Attab (remi.attab@gmail.com), 18 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

static void ux_log_free(void *);
static void ux_log_hide(void *);
static void ux_log_update(void *);
static void ux_log_event(void *);
static void ux_log_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// ux_log
// -----------------------------------------------------------------------------


struct ux_logi
{
    struct
    {
        struct coord star;
        im_id id;
    } state;

    struct ui_label time;
    struct ui_link star;
    struct ui_link id;
    struct ui_label key;
    struct ui_label value;
};

struct ux_log
{
    struct coord coord;
    struct dim cell;

    struct ui_panel *panel;
    struct ui_scroll scroll;

    unit col[2];

    size_t len;
    struct ux_logi items[world_log_cap];
};

struct ux_logi ux_logi_alloc(void)
{
    struct ux_logi ux = {
        .time = ui_label_new(ui_str_v(10)),
        .star = ui_link_new(ui_str_v(symbol_cap)),
        .key = ui_label_new(ui_str_v(symbol_cap)),
        .id = ui_link_new(ui_str_v(im_id_str_len)),
        .value = ui_label_new(ui_str_v(symbol_cap)),
    };

    return ux;
}

// should match the sizes in ux_logi_alloc
static constexpr unit ux_logi_col[3] = {
    [0] = 10,
    [1] = symbol_cap,
    [2] = symbol_cap,
};
static_assert(symbol_cap >= im_id_str_len); // ux_logi_col[1]

void ux_log_alloc(struct ux_view_state *state)
{
    struct dim cell = engine_cell();
    cell.h *= 2;

    unit w = 0;
    for (size_t i = 0; i < array_len(ux_logi_col); ++i)
        w += cell.w * (ux_logi_col[i] + 1);

    struct ux_log *ux = calloc(1, sizeof(*ux));
    *ux = (struct ux_log) {
        .cell = cell,
        .panel = ui_panel_title(make_dim(w, ui_layout_inf), ui_str_c("log")),
        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), cell),
    };

    for (size_t i = 0; i < array_len(ux->items); ++i)
        ux->items[i] = ux_logi_alloc();

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_log,
        .slots = ux_slot_left,
        .panel = ux->panel,
        .fn = {
            .free = ux_log_free,
            .hide = ux_log_hide,
            .update_frame = ux_log_update,
            .event = ux_log_event,
            .render = ux_log_render,
        },
    };
}

static void ux_logi_free(struct ux_logi *ux)
{
    ui_label_free(&ux->time);
    ui_link_free(&ux->star);
    ui_link_free(&ux->id);
    ui_label_free(&ux->key);
    ui_label_free(&ux->value);
}

static void ux_log_free(void *state)
{
    struct ux_log *ux = state;

    ui_panel_free(ux->panel);
    ui_scroll_free(&ux->scroll);
    for (size_t i = 0; i < array_len(ux->items); ++i)
        ux_logi_free(ux->items + i);

    free(ux);
}

void ux_log_show(struct coord star)
{
    struct ux_log *ux = ux_state(ux_view_log);

    ux->coord = star;

    ux_log_update(ux);
    ux_show(ux_view_log);
}

static void ux_log_hide(void *state)
{
    struct ux_log *ux = state;
    ux->coord = coord_nil();
}


static void ux_logi_update(struct ux_logi *ux, const struct logi *it)
{
    ui_str_set_u64(&ux->time.str, it->time);

    ux->state.star = it->star;
    ui_str_set_coord_name(&ux->star.str, it->star);

    ux->state.id = it->id;
    ui_str_set_id(&ux->id.str, it->id);

    ui_str_set_atom(&ux->key.str, it->key);
    ui_str_set_atom(&ux->value.str, it->value);
}

static void ux_log_update(void *state)
{
    struct ux_log *ux = state;
    const struct log *logs = proxy_logs();

    if (!coord_is_nil(ux->coord)) {
        struct chunk *chunk = proxy_chunk(ux->coord);
        if (!chunk) { ux->len = 0; return; }
        logs = chunk_logs(chunk);
    }

    size_t index = 0;
    for (const struct logi *it = log_next(logs, NULL);
         it; index++, it = log_next(logs, it))
    {
        assert(index < array_len(ux->items));
        ux_logi_update(ux->items + index, it);
    }

    assert(index <= array_len(ux->items));
    ux->len = index;

    ui_scroll_update_rows(&ux->scroll, ux->len);
}


static void ux_logi_event(struct ux_logi *ux, struct coord star)
{
    if (coord_is_nil(star) && ui_link_event(&ux->star)) {
        ux_star_show(ux->state.star);
        ux_map_show(ux->state.star);
    }

    if (ui_link_event(&ux->id)) {
        struct coord coord = coord_is_nil(star) ? ux->state.star : star;
        ux_item_show(ux->state.id, coord);
        ux_map_goto(coord);
    }
}

static void ux_log_event(void *state)
{
    struct ux_log *ux = state;
    ui_scroll_event(&ux->scroll);

    for (auto ev = ev_select_star(); ev; ev = nullptr)
        if (!coord_is_nil(ux->coord))
            ux_log_show(ev->star);


    size_t first = ui_scroll_first_row(&ux->scroll);
    size_t last = ui_scroll_last_row(&ux->scroll);
    for (size_t i = first; i < last; ++i)
        ux_logi_event(ux->items + i, ux->coord);
}


static void ux_logi_render(
        struct ux_log *ux,
        size_t row,
        struct ui_layout *layout)
{
    struct ux_logi *entry = ux->items + row;

    const render_layer layer = render_layer_push(1);

    {
        struct rect rect = {
            .x = layout->row.pos.x, .y = layout->row.pos.y,
            .w = layout->row.dim.w, .h = ux->cell.h,
        };

        struct rgba bg =
            ev_mouse_in(rect) ? ui_st.rgba.list.hover :
            row % 2 == 1 ? ui_st.rgba.list.selected :
            ui_st.rgba.bg;

        render_rect_fill(layer, bg, rect);
    }

    const unit tab0 = ux_logi_col[0] + 1;
    const unit tab1 = ux_logi_col[1] + 1 + tab0;

    {
        ui_label_render(&entry->time, layout);
        ui_layout_tab(layout, tab0);

        if (!coord_is_nil(entry->state.star))
            ui_link_render(&entry->star, layout);
        else ui_layout_sep_x(layout, entry->star.w.w);
        ui_layout_tab(layout, tab1);

        ui_label_render(&entry->key, layout);
        ui_layout_next_row(layout);
    }

    {
        ui_layout_sep_x(layout, entry->time.w.w);
        ui_layout_tab(layout, tab0);

        ui_link_render(&entry->id, layout);
        ui_layout_tab(layout, tab1);

        ui_label_render(&entry->value, layout);
        ui_layout_next_row(layout);
    }

    render_layer_pop();
}

static void ux_log_render(void *state, struct ui_layout *layout)
{
    struct ux_log *ux = state;

    struct ui_layout inner = ui_scroll_render(&ux->scroll, layout);
    if (ui_layout_is_nil(&inner)) return;

    size_t first = ui_scroll_first_row(&ux->scroll);
    size_t last = ui_scroll_last_row(&ux->scroll);

    for (size_t row = first; row < last; ++row)
        ux_logi_render(ux, row, &inner);
}
