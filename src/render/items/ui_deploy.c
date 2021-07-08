/* ui_deploy.c
   Rémi Attab (remi.attab@gmail.com), 08 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in ui_item.c

// -----------------------------------------------------------------------------
// deploy
// -----------------------------------------------------------------------------

struct ui_deploy
{
    struct ui_label item, item_val;
    struct ui_label loops, loops_val;
    struct ui_label state, state_val;
};

static void ui_deploy_init(struct ui_deploy *ui)
{
    struct font *font = ui_item_font();

    *ui = (struct ui_deploy) {
        .item = ui_label_new(font, ui_str_c("item:  ")),
        .item_val = ui_label_new(font, ui_str_v(item_str_len)),

        .loops = ui_label_new(font, ui_str_c("loops: ")),
        .loops_val = ui_label_new(font, ui_str_v(4)),

        .state = ui_label_new(font, ui_str_c("state: ")),
        .state_val = ui_label_new(font, ui_str_v(8)),
    };
}

static void ui_deploy_free(struct ui_deploy *ui)
{
    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);
}

static void ui_deploy_update(struct ui_deploy *ui, struct deploy *state)
{
    if (state->item)
        ui_str_set_item(&ui->item_val.str, state->item);
    else ui_str_setc(&ui->item_val.str, "nil");

    if (state->loops != loops_inf)
        ui_str_set_u64(&ui->loops_val.str, state->loops);
    else ui_str_setc(&ui->loops_val.str, "inf");

    ui_str_setc(&ui->state_val.str, state->waiting ? "waiting" : "working");
}

static bool ui_deploy_event(
        struct ui_deploy *ui, struct deploy *state, const SDL_Event *ev)
{
    (void) ui, (void) state, (void) ev;
    return false;
}

static void ui_deploy_render(
        struct ui_deploy *ui, struct deploy *state,
        struct ui_layout *layout, SDL_Renderer *renderer)
{
    (void) state;

    ui_label_render(&ui->item, layout, renderer);
    ui_label_render(&ui->item_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->loops, layout, renderer);
    ui_label_render(&ui->loops_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->state, layout, renderer);
    ui_label_render(&ui->state_val, layout, renderer);
    ui_layout_next_row(layout);
}
