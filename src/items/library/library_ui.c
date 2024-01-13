/* library_ui.c
   Rémi Attab (remi.attab@gmail.com), 24 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// library
// -----------------------------------------------------------------------------

struct ui_library
{
    struct ui_label op, op_val;

    struct ui_label tape;
    struct ui_link tape_val;

    struct ui_label index, index_val, index_of, index_cap;

    struct ui_label value;
    struct ui_link value_val;

    struct im_library state;
};

static void *ui_library_alloc(void)
{
    struct ui_library *ui = mem_alloc_t(ui);

    *ui = (struct ui_library) {
        .op = ui_label_new(ui_str_c("op:    ")),
        .op_val = ui_label_new(ui_str_v(8)),

        .tape = ui_label_new(ui_str_c("tape:  ")),
        .tape_val = ui_link_new(ui_str_v(item_str_len)),

        .index = ui_label_new(ui_str_c("index: ")),
        .index_val = ui_label_new(ui_str_v(3)),
        .index_of = ui_label_new(ui_str_c(" of ")),
        .index_cap = ui_label_new(ui_str_v(3)),

        .value = ui_label_new(ui_str_c("value: ")),
        .value_val = ui_link_new(ui_str_v(item_str_len)),
    };

    return ui;
}

static void ui_library_free(void *_ui)
{
    struct ui_library *ui = _ui;

    ui_label_free(&ui->op);
    ui_label_free(&ui->op_val);

    ui_label_free(&ui->tape);
    ui_link_free(&ui->tape_val);

    ui_label_free(&ui->index);
    ui_label_free(&ui->index_val);
    ui_label_free(&ui->index_of);
    ui_label_free(&ui->index_cap);

    ui_label_free(&ui->value);
    ui_link_free(&ui->value_val);

    mem_free(ui);
}

static void ui_library_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_library *ui = _ui;

    bool ok = chunk_copy(chunk, id, &ui->state, sizeof(ui->state));
    assert(ok);

    switch (ui->state.op) {

    case im_library_nil: {
        ui_set_nil(&ui->op_val);
        ui_set_nil(&ui->tape_val);
        ui_set_nil(&ui->index_val);
        ui_set_nil(&ui->index_cap);
        ui_set_nil(&ui->value_val);
        return;
    }

    case im_library_in:  {
        ui_str_setc(ui_set(&ui->op_val), "inputs");
        ui->op_val.s.fg = ui_st.rgba.in;
        break;
    }
    case im_library_out: {
        ui_str_setc(ui_set(&ui->op_val), "outputs");
        ui->op_val.s.fg = ui_st.rgba.out;
        break;
    }
    case im_library_tech: {
        ui_str_setc(ui_set(&ui->op_val), "tech");
        ui->op_val.s.fg = ui_st.rgba.work;
        break;
    }

    default: { assert(false); }
    }

    ui_str_set_item(ui_set(&ui->tape_val), ui->state.item);
    ui_str_set_u64(ui_set(&ui->index_val), ui->state.index);
    ui_str_set_u64(ui_set(&ui->index_cap), ui->state.len - 1);

    if (ui->state.value)
        ui_str_set_item(ui_set(&ui->value_val), ui->state.value);
    else ui_set_nil(&ui->value_val);
}

static void ui_library_event(void *_ui)
{
    struct ui_library *ui = _ui;

    if (ui_link_event(&ui->tape_val)) ux_tapes_show(ui->state.item);
    if (ui_link_event(&ui->value_val)) ux_tapes_show(ui->state.value);
}

static void ui_library_render(void *_ui, struct ui_layout *layout)
{
    struct ui_library *ui = _ui;

    ui_label_render(&ui->op, layout);
    ui_label_render(&ui->op_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->tape, layout);
    ui_link_render(&ui->tape_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->index, layout);
    ui_label_render(&ui->index_val, layout);
    ui_label_render(&ui->index_of, layout);
    ui_label_render(&ui->index_cap, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->value, layout);
    ui_link_render(&ui->value_val, layout);
    ui_layout_next_row(layout);
}
