/* port_ui.c
   Rémi Attab (remi.attab@gmail.com), 25 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// port
// -----------------------------------------------------------------------------

struct ui_port
{
    struct ui_label status, status_val;
    struct ui_values status_values;

    struct {
        struct ui_label head;
        struct ui_label item;
        struct ui_label coord;
        struct ui_link coord_val;
    } input;

    struct {
        struct ui_label head;
        struct ui_label item, count;
        struct ui_label target;
        struct ui_link target_val;
    } want;

    struct {
        struct ui_label head;
        struct ui_label item, count;
        struct ui_label origin;
        struct ui_link origin_val;
    } has;

    struct ui_label item, count;

    struct { struct coord coord, target, origin; } state;
};

static void *ui_port_alloc(void)
{
    const struct ui_value status[] = {
        { im_port_idle, "disabled", ui_st.rgba.disabled },
        { im_port_docking, "docking", ui_st.rgba.work },
        { im_port_docked, "docked", ui_st.rgba.fg },
        { im_port_unloading, "unloading", ui_st.rgba.out },
    };

    struct ui_port *ui = mem_alloc_t(ui);
    *ui = (struct ui_port) {
        .status = ui_label_new(ui_str_c("status: ")),
        .status_val = ui_label_new(ui_str_v(10)),
        .status_values = ui_values_new(status, array_len(status)),

        .input = {
            .head = ui_label_new(ui_str_c("input:")),
            .item = ui_label_new_s(&ui_st.label.work, ui_str_v(item_str_len)),
            .coord = ui_label_new(ui_str_c("  coord:  ")),
            .coord_val = ui_link_new(ui_str_v(symbol_cap)),
        },

        .want = {
            .head = ui_label_new(ui_str_c("want:")),
            .item = ui_label_new_s(&ui_st.label.in, ui_str_v(item_str_len)),
            .count = ui_label_new(ui_str_v(3)),
            .target = ui_label_new(ui_str_c("  target: ")),
            .target_val = ui_link_new(ui_str_v(symbol_cap)),
        },

        .has = {
            .head = ui_label_new(ui_str_c("has:")),
            .item = ui_label_new_s(&ui_st.label.out, ui_str_v(item_str_len)),
            .count = ui_label_new(ui_str_v(3)),
            .origin = ui_label_new(ui_str_c("  origin: ")),
            .origin_val = ui_link_new(ui_str_v(symbol_cap)),
        },

        .item = ui_label_new(ui_str_c("  item:   ")),
        .count = ui_label_new(ui_str_c("  count:  ")),
    };

    return ui;
}

static void ui_port_free(void *_ui)
{
    struct ui_port *ui = _ui;

    ui_label_free(&ui->status);
    ui_label_free(&ui->status_val);
    ui_values_free(&ui->status_values);

    ui_label_free(&ui->input.head);
    ui_label_free(&ui->input.item);
    ui_label_free(&ui->input.coord);
    ui_link_free(&ui->input.coord_val);

    ui_label_free(&ui->want.head);
    ui_label_free(&ui->want.item);
    ui_label_free(&ui->want.count);
    ui_label_free(&ui->want.target);
    ui_link_free(&ui->want.target_val);

    ui_label_free(&ui->has.head);
    ui_label_free(&ui->has.item);
    ui_label_free(&ui->has.count);
    ui_label_free(&ui->has.origin);
    ui_link_free(&ui->has.origin_val);

    ui_label_free(&ui->item);
    ui_label_free(&ui->count);

    mem_free(ui);
}

static void ui_port_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_port *ui = _ui;

    const struct im_port *port = chunk_get(chunk, id);
    assert(port);

    ui->state.coord = port->input.coord;
    ui->state.target = port->target;
    ui->state.origin = port->origin;

    ui_values_set(&ui->status_values, &ui->status_val, port->state);

    if (!port->input.item) ui_set_nil(&ui->input.item);
    else ui_str_set_item(ui_set(&ui->input.item), port->input.item);

    if (coord_is_nil(port->input.coord)) ui_set_nil(&ui->input.coord_val);
    else ui_str_set_coord_name(ui_set(&ui->input.coord_val), port->input.coord);

    if (!port->want.item) {
        ui_set_nil(&ui->want.item);
        ui_set_nil(&ui->want.count);
    }
    else {
        ui_str_set_item(ui_set(&ui->want.item), port->want.item);
        ui_str_set_u64(ui_set(&ui->want.count), port->want.count);
    }

    if (coord_is_nil(port->target))
        ui_str_setc(&ui->want.target_val.str, "origin");
    else ui_str_set_coord_name(&ui->want.target_val.str, port->target);

    if (!port->has.item) {
        ui_set_nil(&ui->has.item);
        ui_set_nil(&ui->has.count);
    }
    else {
        ui_str_set_item(ui_set(&ui->has.item), port->has.item);
        ui_str_set_u64(ui_set(&ui->has.count), port->has.count);
    }

    if (coord_is_nil(port->origin)) ui_set_nil(&ui->has.origin_val);
    else ui_str_set_coord_name(ui_set(&ui->has.origin_val), port->origin);
}

static void ui_port_event(void *_ui)
{
    struct ui_port *ui = _ui;

    if (ui_link_event(&ui->input.coord_val)) {
        if (!coord_is_nil(ui->state.coord)) {
            ux_star_show(ui->state.coord);
            ux_map_show(ui->state.coord);
        }
    }

    if (ui_link_event(&ui->want.target_val)) {
        if (!coord_is_nil(ui->state.target)) {
            ux_star_show(ui->state.target);
            ux_map_show(ui->state.target);
        }
    }

    if (ui_link_event(&ui->has.origin_val)) {
        if (!coord_is_nil(ui->state.origin)) {
            ux_star_show(ui->state.origin);
            ux_map_show(ui->state.origin);
        }
    }
}

static void ui_port_render(void *_ui, struct ui_layout *layout)
{
    struct ui_port *ui = _ui;

    ui_label_render(&ui->status, layout);
    ui_label_render(&ui->status_val, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    { // input
        ui_label_render(&ui->input.head, layout);
        ui_layout_next_row(layout);

        ui_label_render(&ui->item, layout);
        ui_label_render(&ui->input.item, layout);
        ui_layout_next_row(layout);

        ui_label_render(&ui->input.coord, layout);
        ui_link_render(&ui->input.coord_val, layout);
        ui_layout_next_row(layout);
    }

    ui_layout_sep_row(layout);

    { // want
        ui_label_render(&ui->want.head, layout);
        ui_layout_next_row(layout);

        ui_label_render(&ui->item, layout);
        ui_label_render(&ui->want.item, layout);
        ui_layout_next_row(layout);

        ui_label_render(&ui->count, layout);
        ui_label_render(&ui->want.count, layout);
        ui_layout_next_row(layout);

        ui_label_render(&ui->want.target, layout);
        ui_link_render(&ui->want.target_val, layout);
        ui_layout_next_row(layout);
    }

    ui_layout_sep_row(layout);

    { // has
        ui_label_render(&ui->has.head, layout);
        ui_layout_next_row(layout);

        ui_label_render(&ui->item, layout);
        ui_label_render(&ui->has.item, layout);
        ui_layout_next_row(layout);

        ui_label_render(&ui->count, layout);
        ui_label_render(&ui->has.count, layout);
        ui_layout_next_row(layout);

        ui_label_render(&ui->has.origin, layout);
        ui_link_render(&ui->has.origin_val, layout);
        ui_layout_next_row(layout);
    }
}
