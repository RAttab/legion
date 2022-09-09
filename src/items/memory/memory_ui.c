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
    struct font *font;

    struct ui_label size, size_val;
    struct ui_scroll scroll;
    struct ui_label data_index, data_val;

    size_t state_len;
    struct im_memory state;
};

static void *ui_memory_alloc(struct font *font)
{
    size_t data_len = im_memory_len_max * sizeof(vm_word);
    struct ui_memory *ui = calloc(1, sizeof(*ui) + data_len);

    *ui = (struct ui_memory) {
        .font = font,

        .size = ui_label_new(font, ui_str_c("size: ")),
        .size_val = ui_label_new(font, ui_str_v(2)),

        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
        .data_index = ui_label_new(font, ui_str_v(2)),
        .data_val = ui_label_new(font, ui_str_v(16)),

        .state_len = sizeof(ui->state) + data_len,
    };

    ui->data_index.fg = rgba_gray(0x88);
    ui->data_index.bg = rgba_gray_a(0x44, 0x88);

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

static void ui_memory_update(void *_ui, struct chunk *chunk, id id)
{
    struct ui_memory *ui = _ui;
    const struct im_memory *state = &ui->state;

    bool ok = chunk_copy(chunk, id, &ui->state, ui->state_len);
    assert(ok);

    ui_str_set_u64(&ui->size_val.str, state->len);
    ui_scroll_update(&ui->scroll, state->len);
}

static bool ui_memory_event(void *_ui, const SDL_Event *ev)
{
    struct ui_memory *ui = _ui;
    enum ui_ret ret = ui_nil;

    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret == ui_consume;

    return false;
}

static void ui_memory_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_memory *ui = _ui;
    const struct im_memory *state = &ui->state;

    ui_label_render(&ui->size, layout, renderer);
    ui_label_render(&ui->size_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, ui->font->glyph_h);

    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    size_t first = ui_scroll_first(&ui->scroll);
    size_t last = ui_scroll_last(&ui->scroll);

    for (size_t i = first; i < last; ++i) {
        ui_str_set_u64(&ui->data_index.str, i);
        ui_label_render(&ui->data_index, &inner, renderer);
        ui_layout_sep_x(&inner, ui->font->glyph_w);

        ui_str_set_hex(&ui->data_val.str, state->data[i]);
        ui_label_render(&ui->data_val, &inner, renderer);
        ui_layout_next_row(&inner);
    }
}
