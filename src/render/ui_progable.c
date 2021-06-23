/* ui_progable.c
   Rémi Attab (remi.attab@gmail.com), 23 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in ui_item.c

// -----------------------------------------------------------------------------
// progable
// -----------------------------------------------------------------------------

struct ui_progable
{
    struct ui_label prog, prog_val;
    struct ui_label state, state_val;
    struct ui_label loops, loops_val;

    struct ui_scroll scroll;
    struct ui_label index, in, out;
};

static void ui_progable_init(struct ui_progable *ui)
{
    struct font *font = ui_item_font();

    *ui = (struct ui_progable) {
        .prog = ui_label_new(font, ui_str_c("prog:  ")),
        .prog_val = ui_label_new(font, ui_str_v(10)),

        .state = ui_label_new(font, ui_str_c("state: ")),
        .state_val = ui_label_new(font, ui_str_c("")),

        .loops = ui_label_new(font, ui_str_c("loops: ")),
        .loops_val = ui_label_new(font, ui_str_v(10)),

        .scroll = ui_scroll_new(make_dim(ui_layout_inf, ui_layout_inf), font->glyph_h),
        .index = ui_label_new(font, ui_str_v(2)),
        .in = ui_label_new(font, ui_str_v(10)),
        .out = ui_label_new(font, ui_str_v(10)),
    };

    ui->index.fg = rgba_gray(0x88);
    ui->index.bg = rgba_gray_a(0x44, 0x88);
    ui->in.fg = rgba_green();
    ui->out.fg = rgba_blue();
}

static void ui_progable_free(struct ui_progable *ui)
{
    ui_label_free(&ui->prog);
    ui_label_free(&ui->prog_val);
    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);
    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);
    ui_scroll_free(&ui->scroll);
    ui_label_free(&ui->index);
    ui_label_free(&ui->in);
    ui_label_free(&ui->out);
}

static void ui_progable_update(struct ui_progable *ui, struct progable *state)
{

    if (!state->prog) ui_str_setc(&ui->prog_val.str, "nil");
    else ui_str_set_u64(&ui->prog_val.str, prog_id(state->prog));

    switch (state->state)
    {
    case progable_nil: {
        ui_str_setc(&ui->state_val.str, "ok");
        ui->state_val.fg = rgba_green();
        break;
    }
    case progable_blocked: {
        ui_str_setc(&ui->state_val.str, "blocked");
        ui->state_val.fg = rgba_blue();
        break;
    }
    case progable_error: {
        ui_str_setc(&ui->state_val.str, "error");
        ui->state_val.fg = rgba_red();
        break;
    }
    default: { assert(false); }
    }

    if (state->loops != progable_loops_inf)
        ui_str_set_u64(&ui->loops_val.str, state->loops);
    else ui_str_setc(&ui->loops_val.str, "inf");

    ui_scroll_update(&ui->scroll, state->prog ? prog_len(state->prog) : 0);
}

static bool ui_progable_event(
        struct ui_progable *ui, struct progable *state, const SDL_Event *ev)
{
    (void) state;

    enum ui_ret ret = ui_nil;
    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret == ui_consume;
    return false;
}

static void ui_progable_render(
        struct ui_progable *ui, struct progable *state,
        struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct font *font = ui_item_font();

    ui_label_render(&ui->prog, layout, renderer);
    ui_label_render(&ui->prog_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->state, layout, renderer);
    ui_label_render(&ui->state_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->loops, layout, renderer);
    ui_label_render(&ui->loops_val, layout, renderer);
    ui_layout_next_row(layout);

    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    size_t first = ui_scroll_first(&ui->scroll);
    size_t last = ui_scroll_last(&ui->scroll);

    for (size_t i = first; i < last; ++i) {
        ui_str_set_u64(&ui->index.str, i);
        ui_label_render(&ui->index, &inner, renderer);
        ui_layout_sep_x(&inner, font->glyph_w);

        struct prog_ret ret = prog_at(state->prog, i);

        struct ui_label *label = NULL;
        switch (ret.state)
        {
        case prog_input: { label = &ui->in; break; }
        case prog_output: { label = &ui->out; break; }
        case prog_eof:
        default: { assert(false); }
        }

        ui_str_set_hex(&label->str, ret.item);
        label->bg = i == state->index ? rgba_gray_a(0x33, 0x88) : rgba_nil();
        ui_label_render(label, &inner, renderer);

        ui_layout_next_row(&inner);
    }
}
