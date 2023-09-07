/* label.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "label.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_label_style_default(struct ui_style *s)
{
    void fg(struct ui_label_style *label, struct rgba fg)
    {
        *label = s->label.base;
        label->fg = fg;
    }

    s->label.base = (struct ui_label_style) {
        .font = s->font.base,
        .fg = s->rgba.fg,
        .bg = s->rgba.bg,
        .disabled = s->rgba.disabled,
    };

    s->label.index = s->label.base;
    s->label.index.fg = s->rgba.index.fg;
    s->label.index.bg = s->rgba.index.bg;

    s->label.bold = s->label.base;
    s->label.bold.font = s->font.bold;

    fg(&s->label.in, s->rgba.in);
    fg(&s->label.out, s->rgba.out);
    fg(&s->label.work, s->rgba.work);
    fg(&s->label.active, s->rgba.active);
    fg(&s->label.waiting, s->rgba.waiting);
    fg(&s->label.error, s->rgba.error);
    fg(&s->label.required, rgba_red());

    fg(&s->label.energy.consumed, s->rgba.energy.consumed);
    fg(&s->label.energy.stored, s->rgba.energy.stored);
    fg(&s->label.energy.need, s->rgba.energy.need);
    fg(&s->label.energy.saved, s->rgba.energy.saved);
    fg(&s->label.energy.fusion, s->rgba.energy.fusion);
    fg(&s->label.energy.solar, s->rgba.energy.solar);
    fg(&s->label.energy.burner, s->rgba.energy.burner);
    fg(&s->label.energy.kwheel, s->rgba.energy.kwheel);
}


// -----------------------------------------------------------------------------
// label
// -----------------------------------------------------------------------------

struct ui_label ui_label_new_s(
        const struct ui_label_style *s,
        struct ui_str str)
{
    return (struct ui_label) {
        .w = ui_widget_new(ui_str_len(&str) * s->font->glyph_w, s->font->glyph_h),
        .s = *s,
        .str = str,
        .disabled = false,
    };
}

struct ui_label ui_label_new(struct ui_str str)
{
    return ui_label_new_s(&ui_st.label.base, str);
}

void ui_label_free(struct ui_label *label)
{
    ui_str_free(&label->str);
}

void ui_label_render(
        struct ui_label *label, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_layout_add(layout, &label->w);

    rgba_render(label->s.bg, renderer);
    SDL_Rect rect = ui_widget_rect(&label->w);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    SDL_Point point = pos_as_point(label->w.pos);
    font_render(
            label->s.font,
            renderer,
            point,
            label->disabled ? label->s.disabled : label->s.fg,
            label->str.str, label->str.len);
}

// -----------------------------------------------------------------------------
// label values
// -----------------------------------------------------------------------------

struct ui_values ui_values_new(const struct ui_value *list, size_t len)
{
    struct ui_values values = {
        .len = len,
        .list = calloc(len, sizeof(*list)),
    };
    memcpy(values.list, list, len * sizeof(*list));
    return values;
}

void ui_values_free(struct ui_values *values)
{
    free(values->list);
}

void ui_values_set(struct ui_values *values, struct ui_label *label, uint64_t user)
{
    const struct ui_value *it = values->list;
    const struct ui_value *end = it + values->len;

    for (; it < end; it++) {
        if (it->user != user) continue;

        label->s.fg = it->fg;
        ui_str_setc(ui_set(label), it->str);
        return;
    }

    ui_set_nil(label);
}
