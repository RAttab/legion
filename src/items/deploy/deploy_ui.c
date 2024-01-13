/* deploy_ui.c
   Rémi Attab (remi.attab@gmail.com), 08 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// deploy
// -----------------------------------------------------------------------------

struct ui_deploy
{
    struct ui_label item, item_val;
    struct ui_label loops, loops_val;
    struct ui_label state, state_val;
};

static void *ui_deploy_alloc(void)
{
    struct ui_deploy *ui = mem_alloc_t(ui);
    *ui = (struct ui_deploy) {
        .item = ui_label_new(ui_str_c("item:  ")),
        .item_val = ui_label_new(ui_str_v(item_str_len)),

        .loops = ui_label_new(ui_str_c("loops: ")),
        .loops_val = ui_loops_new(),

        .state = ui_label_new(ui_str_c("state: ")),
        .state_val = ui_waiting_new(),
    };

    return ui;
}

static void ui_deploy_free(void *_ui)
{
    struct ui_deploy *ui = _ui;

    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);

    mem_free(ui);
}

static void ui_deploy_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_deploy *ui = _ui;

    const struct im_deploy *deploy = chunk_get(chunk, id);
    assert(deploy);

    if (!deploy->item) {
        ui_set_nil(&ui->item_val);
        ui_waiting_idle(&ui->state_val);
    }
    else {
        ui_str_set_item(ui_set(&ui->item_val), deploy->item);
        ui_waiting_set(&ui->state_val, deploy->waiting);
    }

    ui_loops_set(&ui->loops_val, deploy->loops);
}

static void ui_deploy_render(void *_ui, struct ui_layout *layout)
{
    struct ui_deploy *ui = _ui;

    ui_label_render(&ui->item, layout);
    ui_label_render(&ui->item_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->loops, layout);
    ui_label_render(&ui->loops_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->state, layout);
    ui_label_render(&ui->state_val, layout);
    ui_layout_next_row(layout);
}
