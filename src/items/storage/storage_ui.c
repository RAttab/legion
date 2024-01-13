/* storage_ui.c
   Rémi Attab (remi.attab@gmail.com), 08 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// storage
// -----------------------------------------------------------------------------

struct ui_storage
{
    struct ui_label item, item_val;
    struct ui_label count, count_val;
    struct ui_label state, state_val;
};

static void *ui_storage_alloc(void)
{
    struct ui_storage *ui = mem_alloc_t(ui);

    *ui = (struct ui_storage) {
        .item = ui_label_new(ui_str_c("item:  ")),
        .item_val = ui_label_new(ui_str_v(item_str_len)),

        .count = ui_label_new(ui_str_c("count:  ")),
        .count_val = ui_label_new(ui_str_v(4)),

        .state = ui_label_new(ui_str_c("state: ")),
        .state_val = ui_waiting_new(),
    };

    return ui;
}

static void ui_storage_free(void *_ui)
{
    struct ui_storage *ui = _ui;

    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->count);
    ui_label_free(&ui->count_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);

    mem_free(ui);
}

static void ui_storage_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_storage *ui = _ui;

    const struct im_storage *storage = chunk_get(chunk, id);
    assert(storage);

    if (!storage->item) {
        ui_set_nil(&ui->item_val);
        ui_waiting_idle(&ui->state_val);
        ui_set_nil(&ui->count_val);
        return;
    }

    ui_str_set_item(ui_set(&ui->item_val), storage->item);
    ui_str_set_u64(&ui->count_val.str, storage->count);
    ui_waiting_set(&ui->state_val, storage->waiting);
}

static void ui_storage_render(void *_ui, struct ui_layout *layout)
{
    struct ui_storage *ui = _ui;

    ui_label_render(&ui->item, layout);
    ui_label_render(&ui->item_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->count, layout);
    ui_label_render(&ui->count_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->state, layout);
    ui_label_render(&ui->state_val, layout);
    ui_layout_next_row(layout);
}
