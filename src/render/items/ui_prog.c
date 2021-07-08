/* ui_prog.c
   Rémi Attab (remi.attab@gmail.com), 08 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in ui_item.c

// -----------------------------------------------------------------------------
// prog
// -----------------------------------------------------------------------------

struct ui_prog
{
    struct ui_label prog, prog_val;
    struct ui_scroll scroll;
    struct ui_label index, in, out;
};

static void ui_prog_init(struct ui_prog *ui)
{
    struct font *font = ui_item_font();

    *ui = (struct ui_prog) {
        .prog = ui_label_new(font, ui_str_c("prog:  ")),
        .prog_val = ui_label_new(font, ui_str_v(item_str_len)),

        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
        .index = ui_label_new(font, ui_str_v(2)),
        .in = ui_label_new(font, ui_str_v(item_str_len)),
        .out = ui_label_new(font, ui_str_v(item_str_len)),
    };

    ui->index.fg = rgba_gray(0x88);
    ui->index.bg = rgba_gray_a(0x44, 0x88);
    ui->in.fg = rgba_green();
    ui->out.fg = rgba_blue();
}

static void ui_prog_free(struct ui_prog *ui)
{
    ui_label_free(&ui->prog);
    ui_label_free(&ui->prog_val);

    ui_scroll_free(&ui->scroll);
    ui_label_free(&ui->index);
    ui_label_free(&ui->in);
    ui_label_free(&ui->out);
}

static void ui_prog_update(struct ui_prog *ui, prog_packed_t state)
{
    struct prog *prog = prog_packed_ptr(state);
    if (!prog) ui_str_setc(&ui->prog_val.str, "nil");
    else ui_str_set_item(&ui->prog_val.str, prog_id(prog));
    
    ui_scroll_update(&ui->scroll, prog ? prog_len(prog) : 0);
}

static bool ui_prog_event(
        struct ui_prog *ui, prog_packed_t state, const SDL_Event *ev)
{
    (void) state;
    
    enum ui_ret ret = ui_nil;
    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret == ui_consume;
    return false;
}

static void ui_prog_render(
        struct ui_prog *ui, prog_packed_t state,
        struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct font *font = ui_item_font();

    ui_label_render(&ui->prog, layout, renderer);
    ui_label_render(&ui->prog_val, layout, renderer);
    ui_layout_next_row(layout);

    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    size_t first = ui_scroll_first(&ui->scroll);
    size_t last = ui_scroll_last(&ui->scroll);

    for (size_t i = first; i < last; ++i) {
        ui_str_set_u64(&ui->index.str, i);
        ui_label_render(&ui->index, &inner, renderer);
        ui_layout_sep_x(&inner, font->glyph_w);

        const struct prog *prog = prog_packed_ptr(state);
        struct prog_ret ret = prog_at(prog, i);

        struct ui_label *label = NULL;
        switch (ret.state)
        {
        case prog_input: { label = &ui->in; break; }
        case prog_output: { label = &ui->out; break; }
        case prog_eof:
        default: { assert(false); }
        }

        ui_str_set_hex(&label->str, ret.item);
        label->bg = i == prog_packed_it(state) ? rgba_gray(0x44) : rgba_nil();
        ui_label_render(label, &inner, renderer);

        ui_layout_next_row(&inner);
    }
}
