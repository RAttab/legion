/* legion_ui.c
   Rémi Attab (remi.attab@gmail.com), 08 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"


// -----------------------------------------------------------------------------
// legion
// -----------------------------------------------------------------------------

struct ui_legion
{
    enum item type;

    struct ui_label mod, mod_val;
    struct ui_scroll scroll;
    struct ui_label index, cargo;
};

static void *ui_legion_alloc(void)
{
    struct ui_legion *ui = calloc(1, sizeof(*ui));

    *ui = (struct ui_legion) {
        .mod = ui_label_new(ui_str_c("mod: ")),
        .mod_val = ui_label_new(ui_str_v(symbol_cap)),

        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), ui_st.font.dim),
        .index = ui_label_new_s(&ui_st.label.index, ui_str_v(2)),
        .cargo = ui_label_new(ui_str_v(item_str_len)),
    };

    return ui;
}

static void ui_legion_free(void *_ui)
{
    struct ui_legion *ui = _ui;

    ui_label_free(&ui->mod);
    ui_label_free(&ui->mod_val);

    ui_scroll_free(&ui->scroll);
    ui_label_free(&ui->index);
    ui_label_free(&ui->cargo);

    free(ui);
}

static void ui_legion_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_legion *ui = _ui;

    const struct im_legion *state = chunk_get(chunk, id);
    assert(state);

    ui->type = im_id_item(state->id);

    if (!state->mod) ui_set_nil(&ui->mod_val);
    else {
        struct symbol mod = {0};
        proxy_mod_name(mod_major(state->mod), &mod);
        ui_str_set_symbol(ui_set(&ui->mod_val), &mod);
    }

    size_t count = 0;
    for (const enum item *it = im_legion_cargo(ui->type); *it; ++it) count++;
    ui_scroll_update_rows(&ui->scroll, count);
}

static bool ui_legion_event(void *_ui, const SDL_Event *ev)
{
    struct ui_legion *ui = _ui;
    enum ui_ret ret = ui_nil;

    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret == ui_consume;

    return false;
}

static void ui_legion_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_legion *ui = _ui;

    ui_label_render(&ui->mod, layout, renderer);
    ui_label_render(&ui->mod_val, layout, renderer);
    ui_layout_next_row(layout);

    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    size_t first = ui_scroll_first_row(&ui->scroll);
    size_t last = ui_scroll_last_row(&ui->scroll);
    const enum item *list = im_legion_cargo(ui->type);

    for (size_t i = first; i < last; ++i) {
        ui_str_set_u64(&ui->index.str, i);
        ui_label_render(&ui->index, &inner, renderer);
        ui_layout_sep_col(&inner);

        ui_str_set_item(&ui->cargo.str, list[i]);
        ui_label_render(&ui->cargo, &inner, renderer);

        ui_layout_next_row(&inner);
    }
}
