/* ui_db.c
   Rémi Attab (remi.attab@gmail.com), 24 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in ui_item.c


// -----------------------------------------------------------------------------
// db
// -----------------------------------------------------------------------------

struct ui_db
{
    struct ui_label size, size_val;
    struct ui_scroll scroll;
    struct ui_label data_index, data_val;
};

static void ui_db_init(struct ui_db *ui)
{
    struct font *font = ui_item_font();

    *ui = (struct ui_db) {
        .size = ui_label_new(font, ui_str_c("size: ")),
        .size_val = ui_label_new(font, ui_str_v(2)),

        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
        .data_index = ui_label_new(font, ui_str_v(2)),
        .data_val = ui_label_new(font, ui_str_v(16)),
    };

    ui->data_index.fg = rgba_gray(0x88);
    ui->data_index.bg = rgba_gray_a(0x44, 0x88);
}

static void ui_db_free(struct ui_db *ui)
{
    ui_label_free(&ui->size);
    ui_label_free(&ui->size_val);

    ui_scroll_free(&ui->scroll);
    ui_label_free(&ui->data_index);
    ui_label_free(&ui->data_val);
}

static void ui_db_update(struct ui_db *ui, struct db *state)
{
    ui_str_set_u64(&ui->size_val.str, state->len);
    ui_scroll_update(&ui->scroll, state->len);
}

static bool ui_db_event(
        struct ui_db *ui, struct db *state, const SDL_Event *ev)
{
    (void) state;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret == ui_consume;
    return false;
}

static void ui_db_render(
        struct ui_db *ui, struct db *state,
        struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct font *font = ui_item_font();

    ui_label_render(&ui->size, layout, renderer);
    ui_label_render(&ui->size_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, font->glyph_h);

    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    size_t first = ui_scroll_first(&ui->scroll);
    size_t last = ui_scroll_last(&ui->scroll);

    for (size_t i = first; i < last; ++i) {
        ui_str_set_u64(&ui->data_index.str, i);
        ui_label_render(&ui->data_index, &inner, renderer);
        ui_layout_sep_x(&inner, font->glyph_w);

        ui_str_set_u64(&ui->data_val.str, state->data[i]);
        ui_label_render(&ui->data_val, &inner, renderer);
        ui_layout_next_row(&inner);
    }
}
