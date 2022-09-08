/* collider_ui.c
   Rémi Attab (remi.attab@gmail.com), 27 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"
#include "items/ui_tape.h"


// -----------------------------------------------------------------------------
// collider
// -----------------------------------------------------------------------------

struct ui_collider
{
    struct
    {
        enum im_collider_op op;
        tape_packed tape;
    } state;

    struct font *font;

    struct ui_label size, size_val;
    struct ui_label rate, rate_val, rate_pct;

    struct ui_label op, op_val;

    struct ui_label item, item_val;
    struct ui_label loops, loops_val;
    struct ui_label waiting, waiting_val;
    struct ui_tape tape;

    struct ui_label energy, energy_val;
    struct ui_label work, work_left, work_sep, work_cap;
    struct ui_label out, out_left, out_sep, out_cap;
};

static void *ui_collider_alloc(struct font *font)
{
    struct ui_collider *ui = calloc(1, sizeof(*ui));

    *ui = (struct ui_collider) {
        .font = font,

        .size = ui_label_new(font, ui_str_c("size: ")),
        .size_val = ui_label_new(font, ui_str_v(3)),

        .rate = ui_label_new(font, ui_str_c("rate: ")),
        .rate_val = ui_label_new(font, ui_str_v(2)),
        .rate_pct = ui_label_new(font, ui_str_c("%")),

        .op = ui_label_new(font, ui_str_c("op: ")),
        .op_val = ui_label_new(font, ui_str_v(8)),

        .item = ui_label_new(font, ui_str_c("item: ")),
        .item_val = ui_label_new(font, ui_str_v(item_str_len)),

        .loops = ui_label_new(font, ui_str_c("loops: ")),
        .loops_val = ui_label_new(font, ui_str_v(4)),

        .waiting = ui_label_new(font, ui_str_c("state: ")),
        .waiting_val = ui_label_new(font, ui_str_v(8)),

        .energy = ui_label_new(font, ui_str_c("energy: ")),
        .energy_val = ui_label_new(font, ui_str_v(8)),

        .work = ui_label_new(font, ui_str_c("work: ")),
        .work_left = ui_label_new(font, ui_str_v(3)),
        .work_sep = ui_label_new(font, ui_str_c(" / ")),
        .work_cap = ui_label_new(font, ui_str_v(3)),

        .out = ui_label_new(font, ui_str_c("outputs: ")),
        .out_left = ui_label_new(font, ui_str_v(3)),
        .out_sep = ui_label_new(font, ui_str_c(" / ")),
        .out_cap = ui_label_new(font, ui_str_v(3)),
    };

    ui_tape_init(&ui->tape, font);
    return ui;
}

static void ui_collider_free(void *_ui)
{
    struct ui_collider *ui = _ui;

    ui_tape_free(&ui->tape);

    ui_label_free(&ui->size);
    ui_label_free(&ui->size_val);

    ui_label_free(&ui->op);
    ui_label_free(&ui->op_val);

    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);

    ui_label_free(&ui->waiting);
    ui_label_free(&ui->waiting_val);

    ui_label_free(&ui->energy);
    ui_label_free(&ui->energy_val);

    ui_label_free(&ui->work);
    ui_label_free(&ui->work_left);
    ui_label_free(&ui->work_sep);
    ui_label_free(&ui->work_cap);

    ui_label_free(&ui->out);
    ui_label_free(&ui->out_left);
    ui_label_free(&ui->out_sep);
    ui_label_free(&ui->out_cap);

    free(ui);
}

static void ui_collider_update(void *_ui, struct chunk *chunk, id id)
{
    struct ui_collider *ui = _ui;

    const struct im_collider *state = chunk_get(chunk, id);
    assert(state);

    ui_str_set_u64(&ui->size_val.str, state->size);
    ui_str_set_u64(&ui->rate_val.str, im_collider_rate(state->size));

    ui->state.op = state->op;
    ui->state.tape = 0;

    switch (state->op)
    {

    case im_collider_nil: {
        ui_str_setc(&ui->op_val.str, "nil");
        break;
    }

    case im_collider_grow: {
        ui_str_setc(&ui->op_val.str, "grow");
        ui_str_set_item(&ui->item_val.str, im_collider_grow_item);
        break;
    }

    case im_collider_in: {
        ui_str_setc(&ui->op_val.str, "input");
        ui_tape_update(&ui->tape, state->tape);
        ui->state.tape = state->tape;
        break;
    }

    case im_collider_work: {
        ui_str_setc(&ui->op_val.str, "working");

        const struct tape *tape = tape_packed_ptr(state->tape);
        ui_str_set_u64(&ui->energy_val.str, tape_energy(tape));

        ui_str_set_u64(&ui->work_left.str, state->work.left);
        ui_str_set_u64(&ui->work_cap.str, state->work.cap);
        break;
    }

    case im_collider_out: {
        ui_str_setc(&ui->op_val.str, "output");
        ui_str_set_item(&ui->item_val.str, state->out.item);
        ui_str_set_u64(&ui->out_left.str, state->out.it + 1);
        ui_str_set_u64(&ui->out_cap.str, state->out.len);
        break;
    }

    default: { assert(false); }

    }

    if (state->loops != loops_inf)
        ui_str_set_u64(&ui->loops_val.str, state->loops);
    else ui_str_setc(&ui->loops_val.str, "inf");

    ui_str_setc(&ui->waiting_val.str, state->waiting ? "waiting" : "working");
}

static bool ui_collider_event(void *_ui, const SDL_Event *ev)
{
    struct ui_collider *ui = _ui;

    if (ui->state.op == im_collider_in)
        return ui_tape_event(&ui->tape, ui->state.tape, ev);
    return false;
}

static void ui_collider_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_collider *ui = _ui;

    ui_label_render(&ui->size, layout, renderer);
    ui_label_render(&ui->size_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->rate, layout, renderer);
    ui_label_render(&ui->rate_val, layout, renderer);
    ui_label_render(&ui->rate_pct, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, ui->font->glyph_h);

    ui_label_render(&ui->op, layout, renderer);
    ui_label_render(&ui->op_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->loops, layout, renderer);
    ui_label_render(&ui->loops_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, ui->font->glyph_h);

    switch (ui->state.op)
    {

    case im_collider_nil: { break; }

    case im_collider_grow: {
        ui_label_render(&ui->waiting, layout, renderer);
        ui_label_render(&ui->waiting_val, layout, renderer);
        ui_layout_next_row(layout);

        ui_label_render(&ui->item, layout, renderer);
        ui_label_render(&ui->item_val, layout, renderer);
        ui_layout_next_row(layout);
        break;
    }

    case im_collider_in: {
        ui_label_render(&ui->waiting, layout, renderer);
        ui_label_render(&ui->waiting_val, layout, renderer);
        ui_layout_next_row(layout);

        ui_layout_sep_y(layout, ui->font->glyph_h);

        ui_tape_render(&ui->tape, ui->state.tape, layout, renderer);
        break;
    }

    case im_collider_work: {
        ui_label_render(&ui->work, layout, renderer);
        ui_label_render(&ui->work_left, layout, renderer);
        ui_label_render(&ui->work_sep, layout, renderer);
        ui_label_render(&ui->work_cap, layout, renderer);
        ui_layout_next_row(layout);

        ui_label_render(&ui->energy, layout, renderer);
        ui_label_render(&ui->energy_val, layout, renderer);
        ui_layout_next_row(layout);
        break;
    }

    case im_collider_out: {
        ui_label_render(&ui->out, layout, renderer);
        ui_label_render(&ui->out_left, layout, renderer);
        ui_label_render(&ui->out_sep, layout, renderer);
        ui_label_render(&ui->out_cap, layout, renderer);
        ui_layout_next_row(layout);

        ui_label_render(&ui->item, layout, renderer);
        ui_label_render(&ui->item_val, layout, renderer);
        ui_layout_next_row(layout);
        break;
    }

    default: { assert(false); }

    }
}
