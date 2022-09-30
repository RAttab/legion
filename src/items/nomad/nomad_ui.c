/* nomad_ui.c
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"

// -----------------------------------------------------------------------------
// nomad
// -----------------------------------------------------------------------------

struct ui_nomad
{
    struct
    {
        vm_word memory[im_nomad_memory_len];
        struct im_nomad_cargo cargo[im_nomad_cargo_len];
    } state;

    struct font *font;

    struct ui_label op, op_val;
    struct ui_values op_values;
    struct ui_label item, item_val;
    struct ui_label loops, loops_val;
    struct ui_label waiting, waiting_val, waiting_values;

    struct ui_label mod, mod_val;

    struct ui_label memory, memory_index, memory_val;
    struct ui_label cargo, cargo_count, cargo_item;
};

static void *ui_nomad_alloc(struct font *font)
{
    const struct ui_value ops[] = {
        { im_nomad_pack, "pack", ui_st.rgba.in },
        { im_nomad_load, "load", ui_st.rgba.in },
        { im_nomad_unload, "unload", ui_st.rgba.out },
    };

    struct ui_nomad *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_nomad) {
        .font = font,

        .op = ui_label_new(ui_str_c("op:    ")),
        .op_val = ui_label_new(ui_str_v(8)),
        .op_values = ui_values_new(ops, array_len(ops)),

        .item = ui_label_new(ui_str_c("item:  ")),
        .item_val = ui_label_new(ui_str_v(item_str_len)),

        .loops = ui_label_new(ui_str_c("loops: ")),
        .loops_val = ui_loops_new(),

        .waiting = ui_label_new(ui_str_c("state: ")),
        .waiting_val = ui_waiting_new(),

        .mod = ui_label_new(ui_str_c("mod: ")),
        .mod_val = ui_label_new(ui_str_v(symbol_cap)),

        .memory = ui_label_new(ui_str_c("memory: ")),
        .memory_index = ui_label_new_s(&ui_st.label.index, ui_str_v(2)),
        .memory_val = ui_label_new(ui_str_v(16)),

        .cargo = ui_label_new(ui_str_c("cargo: ")),
        .cargo_count = ui_label_new_s(&ui_st.label.index, ui_str_v(3)),
        .cargo_item = ui_label_new(ui_str_v(item_str_len)),
    };

    return ui;
}

static void ui_nomad_free(void *_ui)
{
    struct ui_nomad *ui = _ui;

    ui_label_free(&ui->op);
    ui_label_free(&ui->op_val);
    ui_values_free(&ui->op_values);

    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);

    ui_label_free(&ui->waiting);
    ui_label_free(&ui->waiting_val);
    ui_label_free(&ui->waiting_values);

    ui_label_free(&ui->mod);
    ui_label_free(&ui->mod_val);

    ui_label_free(&ui->memory);
    ui_label_free(&ui->memory_index);
    ui_label_free(&ui->memory_val);

    ui_label_free(&ui->cargo);
    ui_label_free(&ui->cargo_count);
    ui_label_free(&ui->cargo_item);

    free(ui);
}

static void ui_nomad_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_nomad *ui = _ui;

    const struct im_nomad *nomad = chunk_get(chunk, id);
    assert(nomad);

    ui_values_set(&ui->op_values, &ui->op_val, nomad->op);

    switch (nomad->op) {
    case im_nomad_nil: { break; }
    case im_nomad_pack: { break; }
    case im_nomad_load: { ui->item_val.s.fg = ui_st.rgba.in; break; }
    case im_nomad_unload: { ui->item_val.s.fg = ui_st.rgba.out; break; }
    default: { assert(false); }
    }

    if (nomad->item) {
        ui_str_set_item(ui_set(&ui->item_val), nomad->item);
        ui_waiting_set(&ui->waiting_val, nomad->waiting);
    }
    else {
        ui_set_nil(&ui->item_val);
        ui_waiting_idle(&ui->waiting_val);
    }

    ui_loops_set(&ui->loops_val, nomad->loops);

    if (!nomad->mod) ui_set_nil(&ui->mod_val);
    else {
        struct symbol mod = {0};
        proxy_mod_name(render.proxy, mod_major(nomad->mod), &mod);
        ui_str_set_symbol(ui_set(&ui->mod_val), &mod);
    }

    memcpy(ui->state.memory, nomad->memory, sizeof(nomad->memory));
    memcpy(ui->state.cargo, nomad->cargo, sizeof(nomad->cargo));
}

static void ui_nomad_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_nomad *ui = _ui;

    ui_label_render(&ui->op, layout, renderer);
    ui_label_render(&ui->op_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->item, layout, renderer);
    ui_label_render(&ui->item_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->loops, layout, renderer);
    ui_label_render(&ui->loops_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->waiting, layout, renderer);
    ui_label_render(&ui->waiting_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, ui->font->glyph_h);

    ui_label_render(&ui->mod, layout, renderer);
    ui_label_render(&ui->mod_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, ui->font->glyph_h);

    {
        ui_label_render(&ui->memory, layout, renderer);
        ui_layout_next_row(layout);

        for (size_t i = 0; i < array_len(ui->state.memory); ++i) {
            ui_str_set_u64(&ui->memory_index.str, i);
            ui_label_render(&ui->memory_index, layout, renderer);

            ui_layout_sep_x(layout, ui->font->glyph_w);

            ui_str_set_hex(&ui->memory_val.str, ui->state.memory[i]);
            ui_label_render(&ui->memory_val, layout, renderer);
            ui_layout_next_row(layout);
        }
    }

    ui_layout_sep_y(layout, ui->font->glyph_h);

    ui_label_render(&ui->cargo, layout, renderer);
    ui_layout_next_row(layout);

    for (size_t i = 0; i < im_nomad_cargo_len; ++i) {
        struct im_nomad_cargo *cargo = ui->state.cargo + i;

        ui_str_set_u64(&ui->cargo_count.str, cargo->count);
        ui_label_render(&ui->cargo_count, layout, renderer);

        ui_layout_sep_x(layout, ui->font->glyph_w);

        ui_str_set_item(&ui->cargo_item.str, cargo->item);
        ui_label_render(&ui->cargo_item, layout, renderer);
        ui_layout_next_row(layout);
    }
}
