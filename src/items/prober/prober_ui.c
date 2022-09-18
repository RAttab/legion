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
    struct im_prober state;

    struct font *font;
    struct ui_label status, status_val;
    struct ui_label work, work_sep, work_left, work_cap;
    struct ui_label target, target_item;
    struct ui_link target_coord;
    struct ui_label result;
    struct ui_link result_val;
};


static void *ui_prober_alloc(struct font *font)
{
    struct ui_prober *ui = calloc(1, sizeof(*ui));

    *ui = (struct ui_prober) {
        .font = font,

        .status = ui_label_new(font, ui_str_c("state:    ")),
        .status_val = ui_label_new(font, ui_str_v(8)),

        .work = ui_label_new(font, ui_str_c("progress: ")),
        .work_sep = ui_label_new(font, ui_str_c(" of ")),
        .work_left = ui_label_new(font, ui_str_v(3)),
        .work_cap = ui_label_new(font, ui_str_v(3)),

        .target = ui_label_new(font, ui_str_c("target:   ")),
        .target_item = ui_label_new(font, ui_str_v(item_str_len)),
        .target_coord = ui_link_new(font, ui_str_v(symbol_cap)),

        .result = ui_label_new(font, ui_str_c("result:   ")),
        .result_val = ui_link_new(font, ui_str_v(16)),
    };

    return ui;
}


static void ui_prober_free(void *_ui)
{
    struct ui_prober *ui = _ui;

    ui_label_free(&ui->status);
    ui_label_free(&ui->status_val);

    ui_label_free(&ui->work);
    ui_label_free(&ui->work_sep);
    ui_label_free(&ui->work_left);
    ui_label_free(&ui->work_cap);

    ui_label_free(&ui->target);
    ui_label_free(&ui->target_item);
    ui_link_free(&ui->target_coord);

    ui_label_free(&ui->result);
    ui_link_free(&ui->result_val);

    free(ui);
}


static void ui_prober_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_prober *ui = _ui;
    const struct im_prober *state = &ui->state;

    bool ok = chunk_copy(chunk, id, &ui->state, sizeof(ui->state));
    assert(ok);


    if (!state->item) {
        ui_str_setc(&ui->status_val.str, "idle");
        ui_str_setc(&ui->target_item.str, "nil");
        ui_str_setc(&ui->target_coord.str, "nil");
        ui_str_setc(&ui->work_cap.str, "inf");
    }
    else {
        if (!state->work.left)
            ui_str_setc(&ui->status_val.str, "done");
        else ui_str_setc(&ui->status_val.str, "probing");

        ui_str_set_item(&ui->target_item.str, state->item);
        ui_str_set_coord_name(&ui->target_coord.str, state->coord);
        ui_str_set_u64(&ui->work_cap.str, state->work.cap);
    }

    ui_str_set_u64(&ui->work_left.str, state->work.left);
    ui_str_set_hex(&ui->result_val.str, state->result);
}


static bool ui_prober_event(void *_ui, const SDL_Event *ev)
{
    struct ui_prober *ui = _ui;
    const struct im_prober *state = &ui->state;

    enum ui_ret ret = ui_nil;

    if ((ret = ui_link_event(&ui->target_coord, ev))) {
        ui_clipboard_copy_hex(&render.ui.board, coord_to_u64(state->coord));
        return ret == ui_consume;
    }

    if ((ret = ui_link_event(&ui->result_val, ev))) {
        ui_str_copy(&ui->result_val.str, &render.ui.board);
        return ret == ui_consume;
    }

    return false;
}


static void ui_prober_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_prober *ui = _ui;

    ui_label_render(&ui->status, layout, renderer);
    ui_label_render(&ui->status_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->work, layout, renderer);
    ui_label_render(&ui->work_left, layout, renderer);
    ui_label_render(&ui->work_sep, layout, renderer);
    ui_label_render(&ui->work_cap, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->target, layout, renderer);
    ui_link_render(&ui->target_coord, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_x(layout, ui->target.str.len * ui->font->glyph_w);
    ui_label_render(&ui->target_item, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->result, layout, renderer);
    ui_link_render(&ui->result_val, layout, renderer);
    ui_layout_next_row(layout);
}
