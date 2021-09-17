/* tape_ui.c
   Rémi Attab (remi.attab@gmail.com), 08 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/ui_tape.h"
#include "utils/str.h"

// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

void ui_tape_init(struct ui_tape *ui, struct font *font)
{
    *ui = (struct ui_tape) {
        .font = font,

        .tape = ui_label_new(font, ui_str_c("tape:   ")),
        .tape_val = ui_label_new(font, ui_str_v(item_str_len)),

        .energy = ui_label_new(font, ui_str_c("energy: ")),
        .energy_val = ui_label_new(font, ui_str_v(str_scaled_len)),

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

void ui_tape_free(struct ui_tape *ui)
{
    ui_label_free(&ui->tape);
    ui_label_free(&ui->tape_val);

    ui_label_free(&ui->energy);
    ui_label_free(&ui->energy_val);

    ui_scroll_free(&ui->scroll);
    ui_label_free(&ui->index);
    ui_label_free(&ui->in);
    ui_label_free(&ui->out);
}

void ui_tape_update(struct ui_tape *ui, tape_packed_t state)
{
    const struct tape *tape = tape_packed_ptr(state);

    if (!tape) ui_str_setc(&ui->tape_val.str, "nil");
    else ui_str_set_item(&ui->tape_val.str, tape_id(tape));

    if (!tape) ui_str_setc(&ui->energy_val.str, "nil");
    else ui_str_set_scaled(&ui->energy_val.str, tape_energy(tape));

    ui_scroll_update(&ui->scroll, tape ? tape_len(tape) : 0);
}

bool ui_tape_event(struct ui_tape *ui, tape_packed_t state, const SDL_Event *ev)
{
    (void) state;
    enum ui_ret ret = ui_nil;

    if ((ret = ui_scroll_event(&ui->scroll, ev))) return ret == ui_consume;

    return false;
}

void ui_tape_render(
        struct ui_tape *ui, tape_packed_t state,
        struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_label_render(&ui->tape, layout, renderer);
    ui_label_render(&ui->tape_val, layout, renderer);
    ui_layout_next_row(layout);

    ui_label_render(&ui->energy, layout, renderer);
    ui_label_render(&ui->energy_val, layout, renderer);
    ui_layout_next_row(layout);

    struct ui_layout inner = ui_scroll_render(&ui->scroll, layout, renderer);
    if (ui_layout_is_nil(&inner)) return;

    size_t first = ui_scroll_first(&ui->scroll);
    size_t last = ui_scroll_last(&ui->scroll);
    const struct tape *tape = tape_packed_ptr(state);

    for (size_t i = first; i < last; ++i) {
        ui_str_set_u64(&ui->index.str, i);
        ui_label_render(&ui->index, &inner, renderer);
        ui_layout_sep_x(&inner, ui->font->glyph_w);

        struct tape_ret ret = tape_at(tape, i);

        struct ui_label *label = NULL;
        switch (ret.state)
        {
        case tape_input: { label = &ui->in; break; }
        case tape_output: { label = &ui->out; break; }
        case tape_eof:
        default: { assert(false); }
        }

        ui_str_set_item(&label->str, ret.item);
        label->bg = i == tape_packed_it(state) ? rgba_gray(0x44) : rgba_nil();
        ui_label_render(label, &inner, renderer);

        ui_layout_next_row(&inner);
    }
}
