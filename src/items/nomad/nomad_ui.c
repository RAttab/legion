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
        word_t data[im_nomad_data_len];
        struct im_nomad_cargo cargo[im_nomad_cargo_len];
    } state;

    struct font *font;

    struct ui_label op, op_val;
    struct ui_label item, item_val;
    struct ui_label loops, loops_val;
    struct ui_label state, state_val;

    struct ui_label mod, mod_val;

    struct ui_label data, data_index, data_val;
    struct ui_label cargo, cargo_count, cargo_item;
    struct ui_scroll scroll;
};

static void *ui_nomad_alloc(struct font *font)
{
    struct ui_nomad *ui = calloc(1, sizeof(*ui));
    *ui = (struct ui_nomad) {
        .font = font,

        .op = ui_label_new(font, ui_str_c("op:    ")),
        .op_val = ui_label_new(font, ui_str_v(8)),

        .item = ui_label_new(font, ui_str_c("item:  ")),
        .item_val = ui_label_new(font, ui_str_v(item_str_len)),

        .loops = ui_label_new(font, ui_str_c("loops: ")),
        .loops_val = ui_label_new(font, ui_str_v(4)),

        .state = ui_label_new(font, ui_str_c("state: ")),
        .state_val = ui_label_new(font, ui_str_v(8)),

        .mod = ui_label_new(font, ui_str_c("mod: ")),
        .mod_val = ui_label_new(font, ui_str_v(symbol_cap)),

        .data = ui_label_new(font, ui_str_c("data: ")),
        .data_index = ui_label_new(font, ui_str_v(2)),
        .data_val = ui_label_new(font, ui_str_v(16)),

        .cargo = ui_label_new(font, ui_str_c("cargo: ")),
        .cargo_count = ui_label_new(font, ui_str_v(3)),
        .cargo_item = ui_label_new(font, ui_str_v(item_str_len)),
        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
    };

    ui->data_index.fg = rgba_gray(0x88);
    ui->data_index.bg = rgba_gray_a(0x44, 0x88);

    ui->cargo_count.fg = rgba_gray(0x88);
    ui->cargo_count.bg = rgba_gray_a(0x44, 0x88);

    return ui;
}

static void ui_nomad_free(void *_ui)
{
    struct ui_nomad *ui = _ui;

    ui_label_free(&ui->op);
    ui_label_free(&ui->op_val);

    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);

    ui_label_free(&ui->mod);
    ui_label_free(&ui->mod_val);

    ui_label_free(&ui->data);
    ui_label_free(&ui->data_index);
    ui_label_free(&ui->data_val);

    ui_label_free(&ui->cargo);
    ui_label_free(&ui->cargo_count);
    ui_label_free(&ui->cargo_item);
    ui_scroll_free(&ui->scroll);

    free(ui);
}

static void ui_nomad_update(void *_ui, struct chunk *chunk, id_t id)
{
    struct ui_nomad *ui = _ui;

    const struct im_nomad *nomad = chunk_get(chunk, id);
    assert(nomad);

    switch (nomad->op) {
    case im_nomad_nil: { ui_str_setc(&ui->op_val.str, "nil"); break; }
    case im_nomad_pack: { ui_str_setc(&ui->op_val.str, "pack"); break; }
    case im_nomad_load: { ui_str_setc(&ui->op_val.str, "load"); break; }
    case im_nomad_unload: { ui_str_setc(&ui->op_val.str, "unload"); break; }
    default: { assert(false); }
    }

    ui->item_val.fg = rgba_white();

    if (nomad->item) {
        ui_str_set_item(&ui->item_val.str, nomad->item);
        ui_str_setc(&ui->state_val.str, nomad->waiting ? "waiting" : "working");

        if (nomad->op == im_nomad_load) ui->item_val.fg = rgba_green();
        if (nomad->op == im_nomad_unload) ui->item_val.fg = rgba_blue();
    }
    else {
        ui_str_setc(&ui->item_val.str, "nil");
        ui_str_setc(&ui->state_val.str, "idle");
    }

    if (nomad->loops != loops_inf)
        ui_str_set_u64(&ui->loops_val.str, nomad->loops);
    else ui_str_setc(&ui->loops_val.str, "inf");

    if (!nomad->mod) ui_str_setc(&ui->mod_val.str, "nil");
    else {
        struct symbol mod = {0};
        proxy_mod_name(render.proxy, mod_maj(nomad->mod), &mod);
        ui_str_set_symbol(&ui->mod_val.str, &mod);
    }

    ui_scroll_update(&ui->scroll, array_len(nomad->cargo));
    memcpy(ui->state.data, nomad->data, sizeof(nomad->data));
    memcpy(ui->state.cargo, nomad->cargo, sizeof(nomad->cargo));
}

static bool ui_nomad_event(void *_ui, const SDL_Event *ev)
{
    struct ui_nomad *ui = _ui;
    enum ui_ret ret = ui_nil;

    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret == ui_consume;

    return false;
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

    ui_label_render(&ui->state, layout, renderer);
    ui_label_render(&ui->state_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, ui->font->glyph_h);

    ui_label_render(&ui->mod, layout, renderer);
    ui_label_render(&ui->mod_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, ui->font->glyph_h);

    {
        ui_label_render(&ui->data, layout, renderer);
        ui_layout_next_row(layout);

        for (size_t i = 0; i < array_len(ui->state.data); ++i) {
            ui_str_set_u64(&ui->data_index.str, i);
            ui_label_render(&ui->data_data, layout, renderer);

            ui_layout_sep_x(&inner, ui->font->glyph_w);

            ui_str_set_hex(&ui->data_val.str, state->data[i]);
            ui_label_render(&ui->data_val, &layout, renderer);
            ui_layout_next_row(&layout);
        }
    }

    ui_layout_sep_y(layout, ui->font->glyph_h);

    {
        ui_label_render(&ui->cargo, layout, renderer);
        ui_layout_next_row(layout);

        struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
        if (ui_layout_is_nil(&inner)) return;

        size_t first = ui_scroll_first(&ui->scroll);
        size_t last = ui_scroll_last(&ui->scroll);

        for (size_t i = first; i < last; ++i) {
            struct im_nomad_cargo *cargo = ui->state.cargo + i;

            ui_str_set_u64(&ui->cargo_count.str, cargo->count);
            ui_label_render(&ui->cargo_count, layout, renderer);

            ui_layout_sep_x(&inner, ui->font->glyph_w);

            ui_str_set_item(&ui->cargo_item.str, cargo->item);
            ui_label_render(&ui->cargo_item, &layout, renderer);
            ui_layout_next_row(&layout);
        }
    }
}
