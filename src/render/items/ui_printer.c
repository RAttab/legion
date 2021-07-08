/* ui_printer.c
   Rémi Attab (remi.attab@gmail.com), 08 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in ui_item.c

// -----------------------------------------------------------------------------
// printer
// -----------------------------------------------------------------------------

struct ui_printer
{
    struct ui_label loops, loops_val;
    struct ui_label state, state_val;
    struct ui_prog prog;
};

static void ui_printer_init(struct ui_printer *ui)
{
    struct font *font = ui_item_font();

    *ui = (struct ui_printer) {
        .loops = ui_label_new(font, ui_str_c("loops: ")),
        .loops_val = ui_label_new(font, ui_str_v(4)),

        .state = ui_label_new(font, ui_str_c("state: ")),
        .state_val = ui_label_new(font, ui_str_v(8)),
    };

    ui_prog_init(&ui->prog);
}

static void ui_printer_free(struct ui_printer *ui)
{
    ui_prog_free(&ui->prog);

    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);
}

static void ui_printer_update(struct ui_printer *ui, struct printer *state)
{
    if (state->loops != loops_inf)
        ui_str_set_u64(&ui->loops_val.str, state->loops);
    else ui_str_setc(&ui->loops_val.str, "inf");

    ui_str_setc(&ui->state_val.str, state->waiting ? "waiting" : "working");

    ui_prog_update(&ui->prog, state->prog);
}

static bool ui_printer_event(
        struct ui_printer *ui, struct printer *state, const SDL_Event *ev)
{
    return ui_prog_event(&ui->prog, state->prog, ev);
}

static void ui_printer_render(
        struct ui_printer *ui, struct printer *state,
        struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct font *font = ui_item_font();

    ui_label_render(&ui->loops, layout, renderer);
    ui_label_render(&ui->loops_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->state, layout, renderer);
    ui_label_render(&ui->state_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_layout_sep_y(layout, font->glyph_h);
    ui_prog_render(&ui->prog, state->prog, layout, renderer);
}
