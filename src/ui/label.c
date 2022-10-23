/* label.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "label.h"


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
