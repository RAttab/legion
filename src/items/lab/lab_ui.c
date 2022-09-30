/* lab_ui.c
   Rémi Attab (remi.attab@gmail.com), 05 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui/ui.h"


// -----------------------------------------------------------------------------
// lab
// -----------------------------------------------------------------------------

struct ui_lab
{
    struct im_lab_bits bits;

    struct font *font;
    struct ui_label item, item_val;
    struct ui_label state, state_val;
    struct ui_label work, work_sep, work_left, work_cap;
    struct ui_label total;
};

struct im_lab_bits im_lab_bits_new(struct font *font)
{
    return (struct im_lab_bits) {
        .margin = make_dim(5, 5),
        .font = font
    };
}

static void *ui_lab_alloc(struct font *font)
{
    struct ui_lab *ui = calloc(1, sizeof(*ui));

    *ui = (struct ui_lab) {
        .bits = im_lab_bits_new(font),
        .font = font,

        .item = ui_label_new(ui_str_c("item:     ")),
        .item_val = ui_label_new_s(&ui_st.label.in, ui_str_v(item_str_len)),

        .state = ui_label_new(ui_str_c("state:    ")),
        .state_val = ui_waiting_new(),

        .work = ui_label_new(ui_str_c("progress: ")),
        .work_sep = ui_label_new(ui_str_c(" of ")),
        .work_left = ui_label_new(ui_str_v(3)),
        .work_cap = ui_loops_new(),

        .total = ui_label_new(ui_str_c("total:    ")),
    };
    return ui;
}


static void ui_lab_free(void *_ui)
{
    struct ui_lab *ui = _ui;

    ui_label_free(&ui->item);
    ui_label_free(&ui->item_val);

    ui_label_free(&ui->state);
    ui_label_free(&ui->state_val);

    ui_label_free(&ui->work);
    ui_label_free(&ui->work_sep);
    ui_label_free(&ui->work_left);
    ui_label_free(&ui->work_cap);

    ui_label_free(&ui->total);

    free(ui);
}

void im_lab_bits_update(
        struct im_lab_bits *ui, const struct tech *tech, enum item item)
{
    if (!item)
        ui->bits = ui->known = 0;
    else {
        ui->bits = im_config_assert(item)->lab_bits;
        ui->known = tech_learned_bits(tech, item);
    }
}

static void ui_lab_update(void *_ui, struct chunk *chunk, im_id id)
{
    struct ui_lab *ui = _ui;

    const struct im_lab *state = chunk_get(chunk, id);
    assert(state);

    im_lab_bits_update(&ui->bits, proxy_tech(render.proxy), state->item);

    if (!state->item) ui_set_nil(&ui->item_val);
    else ui_str_set_item(ui_set(&ui->item_val), state->item);

    switch (state->state) {
    case im_lab_idle: { ui_waiting_idle(&ui->state_val); break; }
    case im_lab_waiting: { ui_waiting_set(&ui->state_val, true); break; }
    case im_lab_working: { ui_waiting_set(&ui->state_val, false); break; }
    default: { assert(false); }
    }

    ui_str_set_u64(&ui->work_left.str, state->work.left);
    ui_loops_set(&ui->work_cap, state->work.cap);
}


void im_lab_bits_render(
        struct im_lab_bits *ui,
        struct ui_layout *layout,
        SDL_Renderer *renderer)
{
    if (!ui->bits) return;

    struct ui_widget w = ui_widget_new(ui_layout_inf, ui->font->glyph_h);
    ui_layout_add(layout, &w);

    SDL_Rect rect = ui_widget_rect(&w);
    rgba_render(rgba_white(), renderer);
    sdl_err(SDL_RenderDrawRect(renderer, &rect));

    rect = (SDL_Rect) {
        .x = rect.x + ui->margin.w,
        .y = rect.y + ui->margin.h,
        .w = (rect.w - 2*ui->margin.w) / ui->bits,
        .h = rect.h - 2*ui->margin.h,
    };

    for (size_t i = 0; i < ui->bits; ++i, rect.x += rect.w) {
        if (!(ui->known & (1ULL << i))) continue;
        sdl_err(SDL_RenderFillRect(renderer, &rect));
    }
}

static void ui_lab_render(
        void *_ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    struct ui_lab *ui = _ui;

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

    im_lab_bits_render(&ui->bits, layout, renderer);
}
