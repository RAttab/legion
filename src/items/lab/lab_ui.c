/* lab_ui.c
   Rémi Attab (remi.attab@gmail.com), 05 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"


// -----------------------------------------------------------------------------
// lab
// -----------------------------------------------------------------------------

struct ui_lab
{
    struct ui_lab_bits bits;

    struct ui_label item, item_val;
    struct ui_label state, state_val;
    struct ui_label work, work_sep, work_left, work_cap;
    struct ui_label total;
};

static void *ui_lab_alloc(void)
{
    struct ui_lab *ui = calloc(1, sizeof(*ui));

    *ui = (struct ui_lab) {
        .bits = ui_lab_bits_new(),

        .item = ui_label_new(ui_str_c("item:     ")),
        .item_val = ui_label_new_s(&ui_st.label.in, ui_str_v(item_str_len)),

        .state = ui_label_new(ui_str_c("state:    ")),
        .state_val = ui_waiting_new(),

        .work = ui_label_new(ui_str_c("progress: ")),
        .work_sep = ui_label_new(ui_str_c(" of ")),
        .work_left = ui_label_new(ui_str_v(3)),
        .work_cap = ui_loops_new(),

        .total = ui_label_new(ui_str_c("total:    ")),
    };
    return ui;
}


static void ui_lab_free(void *_ui)
{
    struct ui_lab *ui = _ui;

    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);

    ui_label_free(&ui->work);
    ui_label_free(&ui->work_sep);
    ui_label_free(&ui->work_left);
    ui_label_free(&ui->work_cap);

    ui_label_free(&ui->total);

    free(ui);
}

static void ui_lab_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_lab *ui = _ui;

    const struct im_lab *state = chunk_get(chunk, id);
    assert(state);

    ui_lab_bits_update(&ui->bits, proxy_tech(), state->item);

    if (!state->item) ui_set_nil(&ui->item_val);
    else ui_str_set_item(ui_set(&ui->item_val), state->item);

    switch (state->state) {
    case im_lab_idle: { ui_waiting_idle(&ui->state_val); break; }
    case im_lab_waiting: { ui_waiting_set(&ui->state_val, true); break; }
    case im_lab_working: { ui_waiting_set(&ui->state_val, false); break; }
    default: { assert(false); }
    }

    ui_str_set_u64(&ui->work_left.str, state->work.left);
    ui_loops_set(&ui->work_cap, state->work.cap);
}


static void ui_lab_render(void *_ui, struct ui_layout *layout)
{
    struct ui_lab *ui = _ui;

    ui_label_render(&ui->item, layout);
    ui_label_render(&ui->item_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->state, layout);
    ui_label_render(&ui->state_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->work, layout);
    ui_label_render(&ui->work_left, layout);
    ui_label_render(&ui->work_sep, layout);
    ui_label_render(&ui->work_cap, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->total, layout);

    ui_lab_bits_render(&ui->bits, layout);
}
