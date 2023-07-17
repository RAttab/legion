/* port_ui.c
   Rémi Attab (remi.attab@gmail.com), 25 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


#include "ui/ui.h"


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
        struct ui_label coord, coord_val;
    } input;

    struct {
        struct ui_label head;
        struct ui_label item, count;
        struct ui_label target, target_val;
    } want;

    struct {
        struct ui_label head;
        struct ui_label item, count;
        struct ui_label origin, origin_val;
    } has;

    struct ui_label item, count;
};

static void *ui_port_alloc(void)
{
    const struct ui_value status[] = {
        { im_port_idle, "disabled", ui_st.rgba.disabled },
        { im_port_docking, "docking", ui_st.rgba.work },
        { im_port_docked, "docked", ui_st.rgba.fg },
        { im_port_unloading, "unloading", ui_st.rgba.out },
    };

    struct ui_port *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_port) {
        .status = ui_label_new(ui_str_c("status: ")),
        .status_val = ui_label_new(ui_str_v(10)),
        .status_values = ui_values_new(status, array_len(status)),

        .input = {
            .head = ui_label_new(ui_str_c("input:")),
            .item = ui_label_new_s(&ui_st.label.work, ui_str_v(item_str_len)),
            .coord = ui_label_new(ui_str_c("  coord:  ")),
            .coord_val = ui_label_new(ui_str_v(symbol_cap)),
        },

        .want = {
            .head = ui_label_new(ui_str_c("want:")),
            .item = ui_label_new_s(&ui_st.label.in, ui_str_v(item_str_len)),
            .count = ui_label_new(ui_str_v(3)),
            .target = ui_label_new(ui_str_c("  target: ")),
            .target_val = ui_label_new(ui_str_v(symbol_cap)),
        },

        .has = {
            .head = ui_label_new(ui_str_c("has:")),
            .item = ui_label_new_s(&ui_st.label.out, ui_str_v(item_str_len)),
            .count = ui_label_new(ui_str_v(3)),
            .origin = ui_label_new(ui_str_c("  origin: ")),
            .origin_val = ui_label_new(ui_str_v(symbol_cap)),
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
    ui_label_free(&ui->input.coord_val);

    ui_label_free(&ui->want.head);
    ui_label_free(&ui->want.item);
    ui_label_free(&ui->want.count);
    ui_label_free(&ui->want.target);
    ui_label_free(&ui->want.target_val);

    ui_label_free(&ui->has.head);
    ui_label_free(&ui->has.item);
    ui_label_free(&ui->has.count);
    ui_label_free(&ui->has.origin);
    ui_label_free(&ui->has.origin_val);

    ui_label_free(&ui->item);
    ui_label_free(&ui->count);

    free(ui);
}

static void ui_port_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_port *ui = _ui;

    const struct im_port *port = chunk_get(chunk, id);
    assert(port);

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

static void ui_port_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_port *ui = _ui;

    ui_label_render(&ui->status, layout, renderer);
    ui_label_render(&ui->status_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    { // input
        ui_label_render(&ui->input.head, layout, renderer);
        ui_layout_next_row(layout);

        ui_label_render(&ui->item, layout, renderer);
        ui_label_render(&ui->input.item, layout, renderer);
        ui_layout_next_row(layout);

        ui_label_render(&ui->input.coord, layout, renderer);
        ui_label_render(&ui->input.coord_val, layout, renderer);
        ui_layout_next_row(layout);
    }

    ui_layout_sep_row(layout);

    { // want
        ui_label_render(&ui->want.head, layout, renderer);
        ui_layout_next_row(layout);

        ui_label_render(&ui->item, layout, renderer);
        ui_label_render(&ui->want.item, layout, renderer);
        ui_layout_next_row(layout);

        ui_label_render(&ui->count, layout, renderer);
        ui_label_render(&ui->want.count, layout, renderer);
        ui_layout_next_row(layout);

        ui_label_render(&ui->want.target, layout, renderer);
        ui_label_render(&ui->want.target_val, layout, renderer);
        ui_layout_next_row(layout);
    }

    ui_layout_sep_row(layout);

    { // has
        ui_label_render(&ui->has.head, layout, renderer);
        ui_layout_next_row(layout);

        ui_label_render(&ui->item, layout, renderer);
        ui_label_render(&ui->has.item, layout, renderer);
        ui_layout_next_row(layout);

        ui_label_render(&ui->count, layout, renderer);
        ui_label_render(&ui->has.count, layout, renderer);
        ui_layout_next_row(layout);

        ui_label_render(&ui->has.origin, layout, renderer);
        ui_label_render(&ui->has.origin_val, layout, renderer);
        ui_layout_next_row(layout);
    }
}
