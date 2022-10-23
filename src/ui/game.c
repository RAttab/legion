/* game.c
   Rémi Attab (remi.attab@gmail.com), 23 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "game.h"
#include "db/specs.h"


// -----------------------------------------------------------------------------
// waiting
// -----------------------------------------------------------------------------

struct ui_label ui_waiting_new(void)
{
    return ui_label_new(ui_str_v(8));
}

void ui_waiting_idle(struct ui_label *label)
{
    label->disabled = true;
    ui_str_setc(&label->str, "idle");
}

void ui_waiting_set(struct ui_label *label, bool waiting)
{
    label->disabled = false;

    if (waiting) {
        ui_str_setc(&label->str, "waiting");
        label->s.fg = ui_st.rgba.waiting;
    }
    else {
        ui_str_setc(&label->str, "working");
        label->s.fg = ui_st.rgba.working;
    }
}


// -----------------------------------------------------------------------------
// loops
// -----------------------------------------------------------------------------

struct ui_label ui_loops_new(void)
{
    return ui_label_new(ui_str_v(3));
}

void ui_loops_set(struct ui_label *label, im_loops loops)
{
    label->disabled = false;

    if (loops != im_loops_inf)
        ui_str_set_u64(&label->str, loops);
    else ui_str_setc(&label->str, "inf");
}


// -----------------------------------------------------------------------------
// lab bits
// -----------------------------------------------------------------------------

struct ui_lab_bits ui_lab_bits_new(void)
{
    return (struct ui_lab_bits) { .margin = make_dim(5, 5) };
}

void ui_lab_bits_update(
        struct ui_lab_bits *ui, const struct tech *tech, enum item item)
{
    if (!item)
        ui->bits = ui->known = 0;
    else {
        ui->bits = specs_var_assert(make_spec(item, spec_lab_bits));
        ui->known = tech_learned_bits(tech, item);
    }
}

void ui_lab_bits_render(
        struct ui_lab_bits *ui,
        struct ui_layout *layout,
        SDL_Renderer *renderer)
{
    if (!ui->bits) return;

    struct ui_widget w = ui_widget_new(ui_layout_inf, ui_st.font.dim.h);
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
