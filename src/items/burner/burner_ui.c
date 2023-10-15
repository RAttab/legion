/* burner_ui.c
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"

// -----------------------------------------------------------------------------
// burner
// -----------------------------------------------------------------------------

struct ui_burner
{
    struct { enum im_burner_op op; } state;

    struct ui_label item, item_val;
    struct ui_label output, output_val;
    struct ui_label loops, loops_val;
    struct ui_label op, op_val;
    struct ui_values op_values;
    struct ui_label waiting, waiting_val;
    struct ui_label work, work_left, work_sep, work_cap;
};

static void *ui_burner_alloc(void)
{
    struct ui_value op[] = {
        {im_burner_in, "ingest", ui_st.rgba.in},
        {im_burner_work, "burning", ui_st.rgba.work},
    };

    struct ui_burner *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_burner) {
        .item = ui_label_new(ui_str_c("item:  ")),
        .item_val = ui_label_new(ui_str_v(item_str_len)),

        .output = ui_label_new(ui_str_c("output: ")),
        .output_val = ui_label_new(ui_str_v(4)),

        .loops = ui_label_new(ui_str_c("loops: ")),
        .loops_val = ui_loops_new(),

        .op = ui_label_new(ui_str_c("op: ")),
        .op_val = ui_label_new(ui_str_v(8)),
        .op_values = ui_values_new(op, array_len(op)),

        .waiting = ui_label_new(ui_str_c("state: ")),
        .waiting_val = ui_waiting_new(),

        .work = ui_label_new(ui_str_c("work: ")),
        .work_left = ui_label_new(ui_str_v(3)),
        .work_sep = ui_label_new(ui_str_c(" / ")),
        .work_cap = ui_label_new(ui_str_v(3)),

    };

    return ui;
}

static void ui_burner_free(void *_ui)
{
    struct ui_burner *ui = _ui;

    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->output);
    ui_label_free(&ui->output_val);

    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);

    ui_label_free(&ui->op);
    ui_label_free(&ui->op_val);
    ui_values_free(&ui->op_values);

    ui_label_free(&ui->waiting);
    ui_label_free(&ui->waiting_val);

    ui_label_free(&ui->work);
    ui_label_free(&ui->work_left);
    ui_label_free(&ui->work_sep);
    ui_label_free(&ui->work_cap);

    free(ui);
}

static void ui_burner_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_burner *ui = _ui;

    const struct im_burner *burner = chunk_get(chunk, id);
    assert(burner);

    ui->state.op = burner->op;

    if (!burner->item) {
        ui_set_nil(&ui->item_val);
        ui_set_nil(&ui->output_val);
        ui_set_nil(&ui->loops_val);
    }
    else {
        ui_str_set_item(ui_set(&ui->item_val), burner->item);
        ui_str_set_u64(ui_set(&ui->output_val), burner->output);
        ui_loops_set(&ui->loops_val, burner->loops);
    }

    ui_waiting_set(&ui->waiting_val, burner->waiting);
    ui_values_set(&ui->op_values, &ui->op_val, ui->state.op);

    if (ui->state.op == im_burner_work) {
        ui_str_set_u64(&ui->work_left.str, burner->work.left);
        ui_str_set_u64(&ui->work_cap.str, burner->work.cap);
    }
}

static void ui_burner_render(void *_ui, struct ui_layout *layout)
{
    struct ui_burner *ui = _ui;

    ui_label_render(&ui->item, layout);
    ui_label_render(&ui->item_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->output, layout);
    ui_label_render(&ui->output_val, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    ui_label_render(&ui->loops, layout);
    ui_label_render(&ui->loops_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->op, layout);
    ui_label_render(&ui->op_val, layout);
    ui_layout_next_row(layout);

    switch (ui->state.op)
    {
    case im_burner_nil: { break; }

    case im_burner_in: {
        ui_label_render(&ui->waiting, layout);
        ui_label_render(&ui->waiting_val, layout);
        ui_layout_next_row(layout);
        break;
    }

    case im_burner_work: {
        ui_label_render(&ui->work, layout);
        ui_label_render(&ui->work_left, layout);
        ui_label_render(&ui->work_sep, layout);
        ui_label_render(&ui->work_cap, layout);
        ui_layout_next_row(layout);
        break;
    }

    default: { assert(false); }
    }
}
