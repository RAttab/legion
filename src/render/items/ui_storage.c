/* ui_storage.c
   Rémi Attab (remi.attab@gmail.com), 08 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in ui_item.c

// -----------------------------------------------------------------------------
// storage
// -----------------------------------------------------------------------------

struct ui_storage
{
    struct ui_label item, item_val;
    struct ui_label count, count_val;
    struct ui_label loops, loops_val;
    struct ui_label state, state_val;
};

static void ui_storage_init(struct ui_storage *ui)
{
    struct font *font = ui_item_font();

    *ui = (struct ui_storage) {
        .item = ui_label_new(font, ui_str_c("item:  ")),
        .item_val = ui_label_new(font, ui_str_v(item_str_len)),

        .item = ui_label_new(font, ui_str_c("count:  ")),
        .item_val = ui_label_new(font, ui_str_v(4)),

        .loops = ui_label_new(font, ui_str_c("loops: ")),
        .loops_val = ui_label_new(font, ui_str_v(4)),

        .state = ui_label_new(font, ui_str_c("state: ")),
        .state_val = ui_label_new(font, ui_str_v(8)),
    };
}

static void ui_storage_free(struct ui_storage *ui)
{
    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->count);
    ui_label_free(&ui->count_val);

    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);
}

static void ui_storage_update(struct ui_storage *ui, struct storage *state)
{
    if (state->item)
        ui_str_set_item(&ui->item_val.str, state->item);
    else ui_str_setc(&ui->item_val.str, "nil");

    ui_str_set_hex(&ui->count_val.str, state->count);

    if (state->loops != loops_inf)
        ui_str_set_u64(&ui->loops_val.str, state->loops);
    else ui_str_setc(&ui->loops_val.str, "inf");

    ui_str_setc(&ui->state_val.str, state->waiting ? "waiting", "working");
}

static bool ui_storage_event(
        struct ui_storage *ui, struct storage *state, const SDL_Event *ev)
{
    (void) ui, (void) state, (void) ev;
    return false;
}

static void ui_storage_render(
        struct ui_storage *ui, struct storage *state,
        struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct font *font = ui_item_font();

    ui_label_render(&ui->item, renderer);
    ui_label_render(&ui->item_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->count, renderer);
    ui_label_render(&ui->count_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->loops, renderer);
    ui_label_render(&ui->loops_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->state, renderer);
    ui_label_render(&ui->state_val, layout, renderer);
    ui_layout_next_row(layout);
}
