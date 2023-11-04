/* printer_ui.c
   Rémi Attab (remi.attab@gmail.com), 08 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// printer
// -----------------------------------------------------------------------------

struct ui_printer
{
    tape_packed tape_state;

    struct ui_label loops, loops_val;
    struct ui_label state, state_val;
    struct ui_tape tape;

};

static void *ui_printer_alloc(void)
{
    struct ui_printer *ui = calloc(1, sizeof(*ui));

    *ui = (struct ui_printer) {
        .loops = ui_label_new(ui_str_c("loops: ")),
        .loops_val = ui_loops_new(),

        .state = ui_label_new(ui_str_c("state: ")),
        .state_val = ui_waiting_new(),
    };

    ui_tape_init(&ui->tape);
    return ui;
}

static void ui_printer_free(void *_ui)
{
    struct ui_printer *ui = _ui;

    ui_tape_free(&ui->tape);

    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);

    free(ui);
}

static void ui_printer_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_printer *ui = _ui;

    const struct im_printer *printer = chunk_get(chunk, id);
    assert(printer);

    ui_loops_set(&ui->loops_val, printer->loops);

    if (!printer->tape) ui_waiting_idle(&ui->state_val);
    else ui_waiting_set(&ui->state_val, printer->waiting);

    ui->tape_state = printer->tape;
    ui_tape_update(&ui->tape, printer->tape);
}

static void ui_printer_event(void *_ui)
{
    struct ui_printer *ui = _ui;
    ui_tape_event(&ui->tape, ui->tape_state);
}

static void ui_printer_render(void *_ui, struct ui_layout *layout)
{
    struct ui_printer *ui = _ui;

    ui_label_render(&ui->loops, layout);
    ui_label_render(&ui->loops_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->state, layout);
    ui_label_render(&ui->state_val, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);
    ui_tape_render(&ui->tape, ui->tape_state, layout);
}
