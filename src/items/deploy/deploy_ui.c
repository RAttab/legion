/* deploy_ui.c
   Rémi Attab (remi.attab@gmail.com), 08 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"

// -----------------------------------------------------------------------------
// deploy
// -----------------------------------------------------------------------------

struct ui_deploy
{
    struct ui_label item, item_val;
    struct ui_label loops, loops_val;
    struct ui_label state, state_val;
};

static void *ui_deploy_alloc(struct font *font)
{
    struct ui_deploy *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_deploy) {
        .item = ui_label_new(font, ui_str_c("item:  ")),
        .item_val = ui_label_new(font, ui_str_v(item_str_len)),

        .loops = ui_label_new(font, ui_str_c("loops: ")),
        .loops_val = ui_label_new(font, ui_str_v(4)),

        .state = ui_label_new(font, ui_str_c("state: ")),
        .state_val = ui_label_new(font, ui_str_v(8)),
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

    free(ui);
}

static void ui_deploy_update(void *_ui, struct chunk *chunk, id id)
{
    struct ui_deploy *ui = _ui;

    const struct im_deploy *deploy = chunk_get(chunk, id);
    assert(deploy);

    if (deploy->item) {
        ui_str_set_item(&ui->item_val.str, deploy->item);
        ui_str_setc(&ui->state_val.str, deploy->waiting ? "waiting" : "working");
    }
    else {
        ui_str_setc(&ui->item_val.str, "nil");
        ui_str_setc(&ui->state_val.str, "idle");
    }

    if (deploy->loops != loops_inf)
        ui_str_set_u64(&ui->loops_val.str, deploy->loops);
    else ui_str_setc(&ui->loops_val.str, "inf");
}

static void ui_deploy_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_deploy *ui = _ui;

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
