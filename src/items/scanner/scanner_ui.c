/* scanner_ui.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// scanner
// -----------------------------------------------------------------------------

struct ui_scanner
{
    struct ui_label status, status_val;
    struct ui_label work, work_sep, work_left, work_cap;
    struct ui_label sector;
    struct ui_link sector_val;
    struct ui_label result, result_val;

    struct { struct coord coord; } state;
};


static void *ui_scanner_alloc(void)
{
    struct ui_scanner *ui = mem_alloc_t(ui);

    *ui = (struct ui_scanner) {
        .status = ui_label_new(ui_str_c("state:    ")),
        .status_val = ui_waiting_new(),

        .work = ui_label_new(ui_str_c("progress: ")),
        .work_sep = ui_label_new(ui_str_c(" of ")),
        .work_left = ui_label_new(ui_str_v(3)),
        .work_cap = ui_label_new(ui_str_v(3)),

        .sector = ui_label_new(ui_str_c("sector: ")),
        .sector_val = ui_link_new(ui_str_v(symbol_cap)),

        .result = ui_label_new(ui_str_c("result: ")),
        .result_val = ui_label_new(ui_str_v(16)),
    };

    return ui;
}


static void ui_scanner_free(void *_ui)
{
    struct ui_scanner *ui = _ui;

    ui_label_free(&ui->status);
    ui_label_free(&ui->status_val);

    ui_label_free(&ui->work);
    ui_label_free(&ui->work_sep);
    ui_label_free(&ui->work_left);
    ui_label_free(&ui->work_cap);

    ui_label_free(&ui->sector);
    ui_link_free(&ui->sector_val);

    ui_label_free(&ui->result);
    ui_label_free(&ui->result_val);

    mem_free(ui);
}


static void ui_scanner_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_scanner *ui = _ui;

    const struct im_scanner *scanner = chunk_get(chunk, id);
    assert(scanner);

    ui->state.coord = scanner->it.coord;

    if (coord_is_nil(scanner->it.coord)) {
        ui_waiting_idle(&ui->status_val);
        ui_set_nil(&ui->sector_val);
        ui_set_nil(&ui->work_cap);
        ui_set_nil(&ui->result_val);
        return;
    }

    ui->status_val.disabled = false;

    if (!scanner->work.left) {
        ui_waiting_set(&ui->status_val, true);
        ui_str_set_hex(ui_set(&ui->result_val), scanner->result);
    }
    else {
        ui_waiting_set(&ui->status_val, false);
        ui_set_nil(&ui->result_val);
    }

    struct symbol name = sector_name(scanner->it.coord, proxy_seed());
    ui_str_set_symbol(ui_set(&ui->sector_val), &name);

    ui_str_set_u64(&ui->work_left.str, scanner->work.left);
    ui_str_set_u64(ui_set(&ui->work_cap), scanner->work.cap);
}


static void ui_scanner_event(void *_ui)
{
    struct ui_scanner *ui = _ui;
    if (ui_link_event(&ui->sector_val))
        ui_clipboard_copy_hex(coord_to_u64(ui->state.coord));
}


static void ui_scanner_render(void *_ui, struct ui_layout *layout)
{
    struct ui_scanner *ui = _ui;

    ui_label_render(&ui->status, layout);
    ui_label_render(&ui->status_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->work, layout);
    if (!coord_is_nil(ui->state.coord)) {
        ui_label_render(&ui->work_left, layout);
        ui_label_render(&ui->work_sep, layout);
    }
    ui_label_render(&ui->work_cap, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    ui_label_render(&ui->sector, layout);
    ui_link_render(&ui->sector_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->result, layout);
    ui_label_render(&ui->result_val, layout);
    ui_layout_next_row(layout);
}
