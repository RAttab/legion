/* prober_ui.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"


// -----------------------------------------------------------------------------
// prober
// -----------------------------------------------------------------------------

struct ui_prober
{
    struct ui_label coord;
    struct ui_link coord_val;
    struct ui_label item, item_val;

    struct ui_label status, status_val;
    struct ui_label work, work_sep, work_left, work_cap;

    struct ui_label result, result_val;

    struct
    {
        enum item item;
        struct coord coord;
    } state;
};


static void *ui_prober_alloc(struct font *font)
{
    (void) font;

    struct ui_prober *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_prober) {
        .coord = ui_label_new(ui_str_c("coord:    ")),
        .coord_val = ui_link_new(ui_str_v(symbol_cap)),

        .item = ui_label_new(ui_str_c("item:     ")),
        .item_val = ui_label_new(ui_str_v(item_str_len)),

        .status = ui_label_new(ui_str_c("state:    ")),
        .status_val = ui_waiting_new(),

        .work = ui_label_new(ui_str_c("progress: ")),
        .work_sep = ui_label_new(ui_str_c(" of ")),
        .work_left = ui_label_new(ui_str_v(3)),
        .work_cap = ui_label_new(ui_str_v(3)),

        .result = ui_label_new(ui_str_c("result:   ")),
        .result_val = ui_label_new(ui_str_v(16)),
    };

    return ui;
}


static void ui_prober_free(void *_ui)
{
    struct ui_prober *ui = _ui;

    ui_label_free(&ui->coord);
    ui_link_free(&ui->coord_val);

    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->status);
    ui_label_free(&ui->status_val);

    ui_label_free(&ui->work);
    ui_label_free(&ui->work_sep);
    ui_label_free(&ui->work_left);
    ui_label_free(&ui->work_cap);

    ui_label_free(&ui->result);
    ui_label_free(&ui->result_val);

    free(ui);
}


static void ui_prober_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_prober *ui = _ui;

    const struct im_prober *prober = chunk_get(chunk, id);
    assert(prober);

    ui->state.item = prober->item;
    ui->state.coord = prober->coord;

    if (!prober->item) {
        ui_set_nil(&ui->coord_val);
        ui_set_nil(&ui->item_val);
        ui_waiting_idle(&ui->status_val);
        ui_set_nil(&ui->work_cap);
        ui_set_nil(&ui->result_val);
        return;
    }

    ui_str_set_item(ui_set(&ui->item_val), prober->item);
    ui_str_set_coord_name(ui_set(&ui->coord_val), prober->coord);

    if (!prober->work.left) {
        ui_waiting_set(&ui->status_val, true);
        ui_str_set_hex(ui_set(&ui->result_val), prober->result);
    }
    else {
        ui_waiting_set(&ui->status_val, false);
        ui_set_nil(&ui->result_val);
    }

    ui_str_set_u64(&ui->work_left.str, prober->work.left);
    ui_str_set_u64(ui_set(&ui->work_cap), prober->work.cap);
}


static bool ui_prober_event(void *_ui, const SDL_Event *ev)
{
    struct ui_prober *ui = _ui;
    enum ui_ret ret = ui_nil;

    if ((ret = ui_link_event(&ui->coord_val, ev))) {
        if (ret != ui_action) return true;
        ui_clipboard_copy_hex(&render.ui.board, coord_to_u64(ui->state.coord));
        return true;
    }

    return false;
}


static void ui_prober_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_prober *ui = _ui;

    ui_label_render(&ui->coord, layout, renderer);
    ui_link_render(&ui->coord_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->item, layout, renderer);
    ui_label_render(&ui->item_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    ui_label_render(&ui->status, layout, renderer);
    ui_label_render(&ui->status_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->work, layout, renderer);
    if (ui->state.item) {
        ui_label_render(&ui->work_left, layout, renderer);
        ui_label_render(&ui->work_sep, layout, renderer);
    }
    ui_label_render(&ui->work_cap, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    ui_label_render(&ui->result, layout, renderer);
    ui_label_render(&ui->result_val, layout, renderer);
    ui_layout_next_row(layout);
}
