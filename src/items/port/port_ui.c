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
    struct ui_label target, target_val;
    struct ui_label want, want_item, want_count;
    struct ui_label has, has_item, has_count;
    struct ui_label sep;
};

static void *ui_port_alloc(struct font *font)
{
    struct ui_port *ui = calloc(1, sizeof(*ui));

    *ui = (struct ui_port) {
        .font = font,

        .status = ui_label_new(font, ui_str_c("status: ")),
        .status_val = ui_label_new(font, ui_str_v(10)),

        .target = ui_label_new(font, ui_str_c("target: ")),
        .target_val = ui_label_new(font, ui_str_v(coord_str_len)),

        .want = ui_label_new(font, ui_str_c("want: ")),
        .want_item = ui_label_new(font, ui_str_v(item_str_len)),
        .want_count = ui_label_new(font, ui_str_v(3)),

        .has = ui_label_new(font, ui_str_c("has:  ")),
        .has_item = ui_label_new(font, ui_str_v(item_str_len)),
        .has_count = ui_label_new(font, ui_str_v(3)),

        .sep = ui_label_new(font, ui_str_c(" x")),
    };

    return ui;
}

static void ui_port_free(void *_ui)
{
    struct ui_port *ui = _ui;

    ui_label_free(&ui->status);
    ui_label_free(&ui->status_val);

    ui_label_free(&ui->target);
    ui_label_free(&ui->target_val);

    ui_label_free(&ui->want);
    ui_label_free(&ui->want_item);
    ui_label_free(&ui->want_count);

    ui_label_free(&ui->has);
    ui_label_free(&ui->has_item);
    ui_label_free(&ui->has_count);

    ui_label_free(&ui->sep);

    free(ui);
}

static void ui_port_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_port *ui = _ui;

    const struct im_port *state = chunk_get(chunk, id);
    assert(state);

    const char *status = "nil";
    if (!state->has.item) status = "waiting";
    else if (state->has.item != state->want.item) status = "unloading";
    else if (state->has.count < state->want.count) status = "loading";
    else status = "launching";
    ui_str_setc(&ui->status_val.str, status);

    if (!coord_is_nil(state->target))
        ui_str_set_coord(&ui->target_val.str, state->target);
    else ui_str_setc(&ui->target_val.str, "nil");

    ui_str_set_item(&ui->want_item.str, state->want.item);
    ui_str_set_u64(&ui->want_count.str, state->want.count);

    ui_str_set_item(&ui->has_item.str, state->has.item);
    ui_str_set_u64(&ui->has_count.str, state->has.count);
}

static void ui_port_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_port *ui = _ui;

    ui_label_render(&ui->status, layout, renderer);
    ui_label_render(&ui->status_val, layout, renderer);
    ui_layout_next_row(layout);
    ui_label_render(&ui->target, layout, renderer);
    ui_label_render(&ui->target_val, layout, renderer);
    ui_layout_next_row(layout);
    ui_layout_sep_y(layout, ui->font->glyph_h);

    ui_label_render(&ui->want, layout, renderer);
    ui_label_render(&ui->want_item, layout, renderer);
    ui_label_render(&ui->sep, layout, renderer);
    ui_label_render(&ui->want_count, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->has, layout, renderer);
    ui_label_render(&ui->has_item, layout, renderer);
    ui_label_render(&ui->sep, layout, renderer);
    ui_label_render(&ui->has_count, layout, renderer);
    ui_layout_next_row(layout);
}
