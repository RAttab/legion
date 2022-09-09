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

    struct font *font;
    struct ui_label item, item_val;
    struct ui_label output, output_val;
    struct ui_label loops, loops_val;
    struct ui_label op, op_val;
    struct ui_label waiting, waiting_val;
    struct ui_label work, work_left, work_sep, work_cap;
};

static void *ui_burner_alloc(struct font *font)
{
    struct ui_burner *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_burner) {
        .font = font,

        .item = ui_label_new(font, ui_str_c("item:  ")),
        .item_val = ui_label_new(font, ui_str_v(item_str_len)),

        .output = ui_label_new(font, ui_str_c("output: ")),
        .output_val = ui_label_new(font, ui_str_v(4)),

        .loops = ui_label_new(font, ui_str_c("loops: ")),
        .loops_val = ui_label_new(font, ui_str_v(4)),

        .op = ui_label_new(font, ui_str_c("op: ")),
        .op_val = ui_label_new(font, ui_str_v(8)),

        .waiting = ui_label_new(font, ui_str_c("state: ")),
        .waiting_val = ui_label_new(font, ui_str_v(8)),

        .work = ui_label_new(font, ui_str_c("work: ")),
        .work_left = ui_label_new(font, ui_str_v(3)),
        .work_sep = ui_label_new(font, ui_str_c(" / ")),
        .work_cap = ui_label_new(font, ui_str_v(3)),

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

    if (burner->item) {
        ui_str_set_item(&ui->item_val.str, burner->item);
        ui_str_set_u64(&ui->output_val.str, burner->output);

        if (burner->loops != loops_inf)
            ui_str_set_u64(&ui->loops_val.str, burner->loops);
        else ui_str_setc(&ui->loops_val.str, "inf");
    }
    else {
        ui_str_setc(&ui->item_val.str, "nil");
        ui_str_setc(&ui->output_val.str, "nil");
        ui_str_setc(&ui->loops_val.str, "nil");
    }

    switch (ui->state.op)
    {
    case im_burner_nil: {
        ui_str_setc(&ui->op_val.str, "nil");
        break;
    }

    case im_burner_in: {
        ui_str_setc(&ui->op_val.str, "input");
        ui_str_setc(&ui->waiting_val.str, burner->waiting ? "waiting" : "working");
        break;
    }

    case im_burner_work: {
        ui_str_setc(&ui->op_val.str, "work");
        ui_str_set_u64(&ui->work_left.str, burner->work.left);
        ui_str_set_u64(&ui->work_cap.str, burner->work.cap);
        break;
    }

    default: { assert(false); }
    }
}

static void ui_burner_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_burner *ui = _ui;

    ui_label_render(&ui->item, layout, renderer);
    ui_label_render(&ui->item_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->output, layout, renderer);
    ui_label_render(&ui->output_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, ui->font->glyph_h);

    ui_label_render(&ui->loops, layout, renderer);
    ui_label_render(&ui->loops_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->op, layout, renderer);
    ui_label_render(&ui->op_val, layout, renderer);
    ui_layout_next_row(layout);

    switch (ui->state.op)
    {
    case im_burner_nil: { break; }

    case im_burner_in: {
        ui_label_render(&ui->waiting, layout, renderer);
        ui_label_render(&ui->waiting_val, layout, renderer);
        ui_layout_next_row(layout);
        break;
    }

    case im_burner_work: {
        ui_label_render(&ui->work, layout, renderer);
        ui_label_render(&ui->work_left, layout, renderer);
        ui_label_render(&ui->work_sep, layout, renderer);
        ui_label_render(&ui->work_cap, layout, renderer);
        ui_layout_next_row(layout);
        break;
    }

    default: { assert(false); }
    }
}
