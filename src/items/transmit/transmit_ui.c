/* transmit_ui.c
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"

// -----------------------------------------------------------------------------
// transmit
// -----------------------------------------------------------------------------

struct ui_transmit
{
    struct ui_label target, target_val;
    struct ui_label channel, channel_val;
};

static void *ui_transmit_alloc(struct font *font)
{
    struct ui_transmit *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_transmit) {
        .target = ui_label_new(font, ui_str_c("target: ")),
        .target_val = ui_label_new(font, ui_str_v(symbol_cap)),

        .channel = ui_label_new(font, ui_str_c("channel: ")),
        .channel_val = ui_label_new(font, ui_str_v(1)),
    };

    return ui;
}

static void ui_transmit_free(void *_ui)
{
    struct ui_transmit *ui = _ui;

    ui_label_free(&ui->target);
    ui_label_free(&ui->target_val);

    ui_label_free(&ui->channel);
    ui_label_free(&ui->channel_val);

    free(ui);
}

static void ui_transmit_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_transmit *ui = _ui;

    const struct im_transmit *transmit = chunk_get(chunk, id);
    assert(transmit);

    ui_str_set_coord_name(&ui->target_val.str, transmit->target);
    ui_str_set_u64(&ui->channel_val.str, transmit->channel);
}

static void ui_transmit_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_transmit *ui = _ui;

    ui_label_render(&ui->target, layout, renderer);
    ui_label_render(&ui->target_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->channel, layout, renderer);
    ui_label_render(&ui->channel_val, layout, renderer);
    ui_layout_next_row(layout);
}
