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
    struct font *font;

    struct ui_label status, status_val;

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

static void *ui_port_alloc(struct font *font)
{
    struct ui_port *ui = calloc(1, sizeof(*ui));

    *ui = (struct ui_port) {
        .font = font,

        .status = ui_label_new(font, ui_str_c("status: ")),
        .status_val = ui_label_new(font, ui_str_v(10)),

        .input = {
            .head = ui_label_new(font, ui_str_c("input:")),
            .item = ui_label_new(font, ui_str_v(item_str_len)),
            .coord = ui_label_new(font, ui_str_c("  coord:  ")),
            .coord_val = ui_label_new(font, ui_str_v(symbol_cap)),
        },

        .want = {
            .head = ui_label_new(font, ui_str_c("want:")),
            .item = ui_label_new(font, ui_str_v(item_str_len)),
            .count = ui_label_new(font, ui_str_v(3)),
            .target = ui_label_new(font, ui_str_c("  target: ")),
            .target_val = ui_label_new(font, ui_str_v(symbol_cap)),
        },

        .has = {
            .head = ui_label_new(font, ui_str_c("has:")),
            .item = ui_label_new(font, ui_str_v(item_str_len)),
            .count = ui_label_new(font, ui_str_v(3)),
            .origin = ui_label_new(font, ui_str_c("  origin: ")),
            .origin_val = ui_label_new(font, ui_str_v(symbol_cap)),
        },

        .item = ui_label_new(font, ui_str_c("  item:   ")),
        .count = ui_label_new(font, ui_str_c("  count:  ")),
    };

    ui->input.item.fg = rgba_yellow();
    return ui;
}

static void ui_port_free(void *_ui)
{
    struct ui_port *ui = _ui;

    ui_label_free(&ui->status);
    ui_label_free(&ui->status_val);

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

    ui->input.item.fg = rgba_white();
    ui->want.item.fg = rgba_white();
    ui->has.item.fg = rgba_white();

    const char *status = "nil";
    switch (port->state) {
    case im_port_idle:    { status = "disabled"; break; }
    case im_port_docking: { status = "docking"; break; }
    case im_port_docked:  { status = "docked"; break; }
    case im_port_loading: {
        ui->want.item.fg = rgba_green();
        status = "loading";
        break;
    }
    case im_port_unloading: {
        ui->has.item.fg = rgba_blue();
        status = "unloading";
        break;
    }
    default: { assert(false); }
    }
    ui_str_setc(&ui->status_val.str, status);

    ui_str_set_item(&ui->input.item.str, port->input.item);
    if (port->input.item) ui->input.item.fg = rgba_yellow();
    ui_str_set_coord_name(&ui->input.coord_val.str, port->input.coord);

    ui_str_set_item(&ui->want.item.str, port->want.item);
    ui_str_set_u64(&ui->want.count.str, port->want.count);
    if (coord_is_nil(port->target))
        ui_str_setc(&ui->want.target_val.str, "origin");
    else ui_str_set_coord_name(&ui->want.target_val.str, port->target);

    ui_str_set_item(&ui->has.item.str, port->has.item);
    ui_str_set_u64(&ui->has.count.str, port->has.count);
    ui_str_set_coord_name(&ui->has.origin_val.str, port->origin);
}

static void ui_port_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_port *ui = _ui;

    ui_label_render(&ui->status, layout, renderer);
    ui_label_render(&ui->status_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, ui->font->glyph_h);

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

    ui_layout_sep_y(layout, ui->font->glyph_h);

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

    ui_layout_sep_y(layout, ui->font->glyph_h);

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
