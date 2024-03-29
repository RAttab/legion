/* collider_ui.c
   Rémi Attab (remi.attab@gmail.com), 27 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// collider
// -----------------------------------------------------------------------------

struct ui_collider
{
    struct
    {
        enum im_collider_op op;
        enum tape_state state;
        tape_packed tape;
    } state;

    struct ui_label size, size_val;
    struct ui_label rate, rate_val, rate_pct;

    struct ui_label op, op_val;

    struct ui_label item, item_val;
    struct ui_label loops, loops_val;
    struct ui_label waiting, waiting_val;

    struct ui_tape tape;
    struct ui_label out, out_left, out_sep, out_cap;
};

static void *ui_collider_alloc(void)
{
    struct ui_collider *ui = mem_alloc_t(ui);

    *ui = (struct ui_collider) {
        .size = ui_label_new(ui_str_c("size: ")),
        .size_val = ui_label_new(ui_str_v(2)),

        .rate = ui_label_new(ui_str_c("rate: ")),
        .rate_val = ui_label_new(ui_str_v(2)),
        .rate_pct = ui_label_new(ui_str_c("%")),

        .op = ui_label_new(ui_str_c("op: ")),
        .op_val = ui_label_new(ui_str_v(8)),

        .item = ui_label_new(ui_str_c("item: ")),
        .item_val = ui_label_new(ui_str_v(item_str_len)),

        .loops = ui_label_new(ui_str_c("loops: ")),
        .loops_val = ui_loops_new(),

        .waiting = ui_label_new(ui_str_c("state: ")),
        .waiting_val = ui_waiting_new(),

        .out = ui_label_new(ui_str_c("outputs: ")),
        .out_left = ui_label_new(ui_str_v(3)),
        .out_sep = ui_label_new(ui_str_c(" / ")),
        .out_cap = ui_label_new(ui_str_v(3)),
    };

    ui_tape_init(&ui->tape);
    return ui;
}

static void ui_collider_free(void *_ui)
{
    struct ui_collider *ui = _ui;

    ui_tape_free(&ui->tape);

    ui_label_free(&ui->size);
    ui_label_free(&ui->size_val);

    ui_label_free(&ui->rate);
    ui_label_free(&ui->rate_val);
    ui_label_free(&ui->rate_pct);

    ui_label_free(&ui->op);
    ui_label_free(&ui->op_val);

    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);

    ui_label_free(&ui->waiting);
    ui_label_free(&ui->waiting_val);

    ui_label_free(&ui->out);
    ui_label_free(&ui->out_left);
    ui_label_free(&ui->out_sep);
    ui_label_free(&ui->out_cap);

    mem_free(ui);
}

static void ui_collider_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_collider *ui = _ui;

    const struct im_collider *collider = chunk_get(chunk, id);
    assert(collider);

    ui_str_set_u64(&ui->size_val.str, collider->size);

    unsigned pct = (100 * im_collider_rate(collider->size)) / im_collider_grow_max;
    ui_str_set_u64(&ui->rate_val.str, pct);

    ui->state.op = collider->op;
    ui->state.tape = 0;

    switch (collider->op)
    {

    case im_collider_nil: {
        ui_set_nil(&ui->op_val);
        break;
    }

    case im_collider_grow: {
        ui->op_val.s.fg = ui_st.rgba.in;
        ui_str_setc(ui_set(&ui->op_val), "grow");

        ui->item_val.s.fg = ui_st.rgba.in;
        ui_str_set_item(&ui->item_val.str, im_collider_grow_item);
        break;
    }

    case im_collider_tape: {
        ui->op_val.s.fg = ui_st.rgba.work;
        ui_str_setc(ui_set(&ui->op_val), "tape");

        const struct tape *tape = tape_packed_ptr(collider->tape);
        struct tape_ret ret = tape_at(tape, tape_packed_it(collider->tape));
        ui->state.state = ret.state;

        switch (ret.state)
        {
        case tape_input:
        case tape_work: {
            ui_tape_update(&ui->tape, collider->tape);
            ui->state.tape = collider->tape;
            break;
        }

        case tape_output: {
            ui->op_val.s.fg = ui_st.rgba.out;
            ui_str_setc(ui_set(&ui->op_val), "output");

            ui->item_val.s.fg = ui_st.rgba.out;
            ui_str_set_item(&ui->item_val.str, collider->out.item);

            ui_str_set_u64(&ui->out_left.str, collider->out.it + 1);
            ui_str_set_u64(&ui->out_cap.str, collider->out.len);
            break;
        }

        case tape_eof:
        default: { assert(false); }
        }

        break;
    }

    default: { assert(false); }
    }

    ui_loops_set(&ui->loops_val, collider->loops);
    ui_waiting_set(&ui->waiting_val, collider->waiting);
}

static void ui_collider_event(void *_ui)
{
    struct ui_collider *ui = _ui;

    if (ui->state.op == im_collider_tape && ui->state.state != tape_output)
        ui_tape_event(&ui->tape, ui->state.tape);
}

static void ui_collider_render(void *_ui, struct ui_layout *layout)
{
    struct ui_collider *ui = _ui;

    ui_label_render(&ui->size, layout);
    ui_label_render(&ui->size_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->rate, layout);
    ui_label_render(&ui->rate_val, layout);
    ui_label_render(&ui->rate_pct, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    ui_label_render(&ui->op, layout);
    ui_label_render(&ui->op_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->loops, layout);
    ui_label_render(&ui->loops_val, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    switch (ui->state.op)
    {

    case im_collider_nil: { break; }

    case im_collider_grow: {
        ui_label_render(&ui->waiting, layout);
        ui_label_render(&ui->waiting_val, layout);
        ui_layout_next_row(layout);

        ui->item_val.s.fg = ui_st.rgba.in;
        ui_label_render(&ui->item, layout);
        ui_label_render(&ui->item_val, layout);
        ui_layout_next_row(layout);
        break;
    }

    case im_collider_tape: {

        switch (ui->state.state)
        {

        case tape_input:
        case tape_work: {
            ui_label_render(&ui->waiting, layout);
            ui_label_render(&ui->waiting_val, layout);
            ui_layout_next_row(layout);

            ui_layout_sep_row(layout);

            ui_tape_render(&ui->tape, ui->state.tape, layout);
            break;
        }

        case tape_output: {
            ui_label_render(&ui->out, layout);
            ui_label_render(&ui->out_left, layout);
            ui_label_render(&ui->out_sep, layout);
            ui_label_render(&ui->out_cap, layout);
            ui_layout_next_row(layout);

            ui->item_val.s.fg = ui_st.rgba.out;
            ui_label_render(&ui->item, layout);
            ui_label_render(&ui->item_val, layout);
            ui_layout_next_row(layout);
            break;
        }

        default: { assert(false); }
        }

        break;
    }

    default: { assert(false); }

    }
}
