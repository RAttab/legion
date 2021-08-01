/* ui_extract.c
   Rémi Attab (remi.attab@gmail.com), 08 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in ui_item.c

// -----------------------------------------------------------------------------
// extract
// -----------------------------------------------------------------------------

struct ui_extract
{
    struct ui_label loops, loops_val;
    struct ui_label state, state_val;
    struct ui_tape tape;
};

static void ui_extract_init(struct ui_extract *ui)
{
    struct font *font = ui_item_font();

    *ui = (struct ui_extract) {
        .loops = ui_label_new(font, ui_str_c("loops: ")),
        .loops_val = ui_label_new(font, ui_str_v(4)),

        .state = ui_label_new(font, ui_str_c("state: ")),
        .state_val = ui_label_new(font, ui_str_v(8)),
    };

    ui_tape_init(&ui->tape);
}

static void ui_extract_free(struct ui_extract *ui)
{
    ui_tape_free(&ui->tape);

    ui_label_free(&ui->loops);
    ui_label_free(&ui->loops_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);
}

static void ui_extract_update(struct ui_extract *ui, struct extract *state)
{
    if (state->loops != loops_inf)
        ui_str_set_u64(&ui->loops_val.str, state->loops);
    else ui_str_setc(&ui->loops_val.str, "inf");

    ui_str_setc(&ui->state_val.str, state->waiting ? "waiting" : "working");

    ui_tape_update(&ui->tape, state->tape);
}

static bool ui_extract_event(
        struct ui_extract *ui, struct extract *state, const SDL_Event *ev)
{
    return ui_tape_event(&ui->tape, state->tape, ev);
}

static void ui_extract_render(
        struct ui_extract *ui, struct extract *state,
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
    ui_tape_render(&ui->tape, state->tape, layout, renderer);
}
