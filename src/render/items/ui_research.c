/* ui_research.c
   Rémi Attab (remi.attab@gmail.com), 05 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

// included in ui_item.c

// -----------------------------------------------------------------------------
// research
// -----------------------------------------------------------------------------

struct ui_research
{
    uint8_t bits;
    uint64_t known;
    struct dim margin;

    struct ui_label item, item_val;
    struct ui_label state, state_val;
    struct ui_label work, work_sep, work_left, work_cap;
    struct ui_label total;
};


static void ui_research_init(struct ui_research *ui)
{
    struct font *font = ui_item_font();

    enum { u8_len = 2, u16_len = 4, u32_len = 8, u64_len = 16 };

    *ui = (struct ui_research) {
        .margin = make_dim(5, 5),

        .item = ui_label_new(font, ui_str_c("item:     ")),
        .item_val = ui_label_new(font, ui_str_v(item_str_len)),

        .state = ui_label_new(font, ui_str_c("state:    ")),
        .state_val = ui_label_new(font, ui_str_v(8)),

        .work = ui_label_new(font, ui_str_c("progress: ")),
        .work_sep = ui_label_new(font, ui_str_c(" of ")),
        .work_left = ui_label_new(font, ui_str_v(3)),
        .work_cap = ui_label_new(font, ui_str_v(3)),

        .total = ui_label_new(font, ui_str_c("total:    ")),
    };
}


static void ui_research_free(struct ui_research *ui)
{
    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);

    ui_label_free(&ui->work);
    ui_label_free(&ui->work_sep);
    ui_label_free(&ui->work_left);
    ui_label_free(&ui->work_cap);

    ui_label_free(&ui->total);
}


static void ui_research_update(
        struct ui_research *ui, struct research *state, struct chunk *chunk)
{
    ui_str_set_item(&ui->item_val.str, state->item);

    if (state->item) {
        const struct tape *tape = tapes_get(state->item);
        assert(tape);

        ui->bits = tape_bits(tape);
        ui->known = chunk_known_bits(chunk, state->item);
    }
    else ui->bits = ui->known = 0;

    switch (state->state)
    {
    case research_idle: {
        ui_str_setc(&ui->state_val.str, "idle");
        ui->state_val.fg = rgba_gray(0x88);
        break;
    }
    case research_waiting: {
        ui_str_setc(&ui->state_val.str, "waiting");
        ui->state_val.fg = rgba_blue();
        break;
    }
    case research_working: {
        ui_str_setc(&ui->state_val.str, "working");
        ui->state_val.fg = rgba_green();
        break;
    }
    default: { assert(false); }
    }

    ui_str_set_u64(&ui->work_left.str, state->work.left);
    if (state->state != research_idle)
        ui_str_set_u64(&ui->work_cap.str, state->work.cap);
    else ui_str_setc(&ui->work_cap.str, "inf");
}


static bool ui_research_event(
        struct ui_research *ui, struct research *state, const SDL_Event *ev)
{
    (void) ui, (void) state, (void) ev;
    return false;
}

static void ui_research_render(
        struct ui_research *ui, struct research *state,
        struct ui_layout *layout, SDL_Renderer *renderer)
{
    (void) state;
    struct font *font = ui_item_font();

    ui_label_render(&ui->item, layout, renderer);
    ui_label_render(&ui->item_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->state, layout, renderer);
    ui_label_render(&ui->state_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->work, layout, renderer);
    ui_label_render(&ui->work_left, layout, renderer);
    ui_label_render(&ui->work_sep, layout, renderer);
    ui_label_render(&ui->work_cap, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->total, layout, renderer);

    {
        struct ui_widget w = ui_widget_new(ui_layout_inf, font->glyph_h);
        ui_layout_add(layout, &w);
        rgba_render(rgba_white(), renderer);

        SDL_Rect rect = ui_widget_rect(&w);
        sdl_err(SDL_RenderDrawRect(renderer, &rect));

        rect = (SDL_Rect) {
            .x = rect.x + ui->margin.w,
            .y = rect.y + ui->margin.h,
            .w = (rect.w - 2*ui->margin.w) / ui->bits,
            .h = rect.h - 2*ui->margin.h,
        };

        for (size_t i = 0; i < ui->bits; ++i, rect.x += rect.w) {
            if (!(ui->bits & (1ULL << i))) continue;
            sdl_err(SDL_RenderDrawRect(renderer, &rect));
        }
    }
}
