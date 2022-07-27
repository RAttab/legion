/* packer_ui.c
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"

// -----------------------------------------------------------------------------
// packer
// -----------------------------------------------------------------------------

struct ui_packer
{
    struct ui_label item, item_val;
    struct ui_label loops, loops_val;
    struct ui_label state, state_val;
};

static void *ui_packer_alloc(struct font *font)
{
    struct ui_packer *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_packer) {
        .item = ui_label_new(font, ui_str_c("item:  ")),
        .item_val = ui_label_new(font, ui_str_v(item_str_len)),

        .loops = ui_label_new(font, ui_str_c("loops: ")),
        .loops_val = ui_label_new(font, ui_str_v(4)),

        .state = ui_label_new(font, ui_str_c("state: ")),
        .state_val = ui_label_new(font, ui_str_v(8)),
    };

    return ui;
}

static void ui_packer_free(void *_ui)
{
    struct ui_packer *ui = _ui;

    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);

    free(ui);
}

static void ui_packer_update(void *_ui, struct chunk *chunk, id_t id)
{
    struct ui_packer *ui = _ui;

    const struct im_packer *packer = chunk_get(chunk, id);
    assert(packer);

    if (packer->item) {
        ui_str_set_item(&ui->item_val.str, packer->item);
        ui_str_setc(&ui->state_val.str, packer->waiting ? "waiting" : "working");
    }
    else {
        ui_str_setc(&ui->item_val.str, "nil");
        ui_str_setc(&ui->state_val.str, "idle");
    }

    if (packer->loops != loops_inf)
        ui_str_set_u64(&ui->loops_val.str, packer->loops);
    else ui_str_setc(&ui->loops_val.str, "inf");
}

static void ui_packer_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_packer *ui = _ui;

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
