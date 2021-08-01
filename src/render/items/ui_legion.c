/* ui_legion.c
   Rémi Attab (remi.attab@gmail.com), 08 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in ui_mod.c

// -----------------------------------------------------------------------------
// legion
// -----------------------------------------------------------------------------

struct ui_legion
{
    struct ui_label mod, mod_val;

    struct ui_scroll scroll;
    struct ui_label index, cargo;
};

static void ui_legion_init(struct ui_legion *ui)
{
    struct font *font = ui_mod_font();

    *ui = (struct ui_legion) {
        .mod = ui_label_new(font, ui_str_c("mod: ")),
        .mod_val = ui_label_new(font, ui_str_v(symbol_cap)),

        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
        .index = ui_label_new(font, ui_str_v(2)),
        .cargo = ui_label_new(font, ui_str_v(item_str_len)),
    };

    ui->index.fg = rgba_gray(0x88);
    ui->index.bg = rgba_gray_a(0x44, 0x88);
}

static void ui_legion_free(struct ui_legion *ui)
{
    ui_label_free(&ui->mod);
    ui_label_free(&ui->mod_val);

    ui_scroll_free(&ui->scroll);
    ui_label_free(&ui->index);
    ui_label_free(&ui->cargo);
}

static void ui_legion_update(struct ui_legion *ui, struct legion *state)
{
    if (!state->mod) ui_str_setc(&ui->mod_val.str, "nil");
    else {
        struct symbol mod = {0};
        mods_name(world_mods(core.state.world), mod_maj(state->mod), &mod);
        ui_str_set_symbol(&ui->mod_val.str, &mod);
    }

    size_t count = 0;
    for (const enum item *it = legion_cargo(id_item(state->id)); *it; ++it) count++;
    ui_scroll_update(&ui->scroll, count);
}

static bool ui_legion_event(
        struct ui_legion *ui, struct legion *state, const SDL_Event *ev)
{
    (void) state;
    
    enum ui_ret ret = ui_nil;
    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret == ui_consume;
    return false;
}

static void ui_legion_render(
        struct ui_legion *ui, struct legion *state,
        struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct font *font = ui_mod_font();

    ui_label_render(&ui->mod, layout, renderer);
    ui_label_render(&ui->mod_val, layout, renderer);
    ui_layout_next_row(layout);

    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    size_t first = ui_scroll_first(&ui->scroll);
    size_t last = ui_scroll_last(&ui->scroll);
    const enum item *list = legion_cargo(id_item(state->id));

    for (size_t i = first; i < last; ++i) {
        ui_str_set_u64(&ui->index.str, i);
        ui_label_render(&ui->index, &inner, renderer);
        ui_layout_sep_x(&inner, font->glyph_w);

        ui_str_set_item(&ui->cargo.str, list[i]);
        ui_label_render(&ui->cargo, &inner, renderer);

        ui_layout_next_row(&inner);
    }
}
