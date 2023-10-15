/* fusion_ui.c
   Rémi Attab (remi.attab@gmail.com), 15 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"

// -----------------------------------------------------------------------------
// fusion
// -----------------------------------------------------------------------------

struct ui_fusion
{
    struct ui_label energy, energy_val;
    struct ui_label state, state_val;
    struct ui_label input, input_val;
};

static void *ui_fusion_alloc(void)
{
    struct ui_fusion *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_fusion) {
        .energy = ui_label_new(ui_str_c("energy: ")),
        .energy_val = ui_label_new(ui_str_v(str_scaled_len)),

        .state = ui_label_new(ui_str_c("state: ")),
        .state_val = ui_label_new_s(&ui_st.label.active, ui_str_v(8)),

        .input = ui_label_new(ui_str_c("input: ")),
        .input_val = ui_label_new_s(&ui_st.label.in, ui_str_v(item_str_len)),
    };

    return ui;
}

static void ui_fusion_free(void *_ui)
{
    struct ui_fusion *ui = _ui;

    ui_label_free(&ui->energy);
    ui_label_free(&ui->energy_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);

    ui_label_free(&ui->input);
    ui_label_free(&ui->input_val);

    free(ui);
}

static void ui_fusion_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_fusion *ui = _ui;

    const struct im_fusion *fusion = chunk_get(chunk, id);
    assert(fusion);

    ui_str_set_scaled(&ui->energy_val.str, fusion->energy);

    if (fusion->paused) ui_set_nil(&ui->state_val);
    else ui_str_setc(ui_set(&ui->state_val), "active");

    if (fusion->waiting) ui_set_nil(&ui->input_val);
    else ui_str_set_item(ui_set(&ui->input_val), im_fusion_input_item);
}

static void ui_fusion_render(void *_ui, struct ui_layout *layout)
{
    struct ui_fusion *ui = _ui;

    ui_label_render(&ui->energy, layout);
    ui_label_render(&ui->energy_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->state, layout);
    ui_label_render(&ui->state_val, layout);
    ui_layout_next_row(layout);

    ui_label_render(&ui->input, layout);
    ui_label_render(&ui->input_val, layout);
    ui_layout_next_row(layout);
}
