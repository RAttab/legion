/* memory_ui.c
   Rémi Attab (remi.attab@gmail.com), 24 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"


// -----------------------------------------------------------------------------
// memory
// -----------------------------------------------------------------------------

struct ui_memory
{
    struct ui_label size, size_val;
    struct ui_scroll scroll;
    struct ui_label data_index, data_val;

    size_t state_len;
    struct im_memory state;
};

static void *ui_memory_alloc(void)
{
    size_t data_len = im_memory_len_max * sizeof(vm_word);
    struct ui_memory *ui = calloc(1, sizeof(*ui) + data_len);

    *ui = (struct ui_memory) {
        .size = ui_label_new(ui_str_c("size: ")),
        .size_val = ui_label_new(ui_str_v(2)),

        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), engine_cell()),
        .data_index = ui_label_new_s(&ui_st.label.index, ui_str_v(2)),
        .data_val = ui_label_new(ui_str_v(16)),

        .state_len = sizeof(ui->state) + data_len,
    };

    return ui;
}

static void ui_memory_free(void *_ui)
{
    struct ui_memory *ui = _ui;

    ui_label_free(&ui->size);
    ui_label_free(&ui->size_val);

    ui_scroll_free(&ui->scroll);
    ui_label_free(&ui->data_index);
    ui_label_free(&ui->data_val);

    free(ui);
}

static void ui_memory_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_memory *ui = _ui;
    const struct im_memory *state = &ui->state;

    bool ok = chunk_copy(chunk, id, &ui->state, ui->state_len);
    assert(ok);

    ui_str_set_u64(&ui->size_val.str, state->len);
    ui_scroll_update_rows(&ui->scroll, state->len);
}

static void ui_memory_event(void *_ui)
{
    struct ui_memory *ui = _ui;
    ui_scroll_event(&ui->scroll);
}

static void ui_memory_render(void *_ui, struct ui_layout *layout)
{
    struct ui_memory *ui = _ui;
    const struct im_memory *state = &ui->state;

    ui_label_render(&ui->size, layout);
    ui_label_render(&ui->size_val, layout);
    ui_layout_next_row(layout);

    ui_layout_sep_row(layout);

    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout);
    if (ui_layout_is_nil(&inner)) return;

    size_t first = ui_scroll_first_row(&ui->scroll);
    size_t last = ui_scroll_last_row(&ui->scroll);

    for (size_t i = first; i < last; ++i) {
        ui_str_set_u64(&ui->data_index.str, i);
        ui_label_render(&ui->data_index, &inner);
        ui_layout_sep_col(&inner);

        ui_str_set_hex(&ui->data_val.str, state->data[i]);
        ui_label_render(&ui->data_val, &inner);
        ui_layout_next_row(&inner);
    }
}
