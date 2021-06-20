/* label.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/font.h"

#include <stdarg.h>


// -----------------------------------------------------------------------------
// label
// -----------------------------------------------------------------------------

struct ui_label ui_label_new(struct font *font, struct ui_str str)
{
    return (struct ui_label) {
        .w = ui_widget_new(ui_str_len(&str) * font->glyph_w, font->glyph_h),
        .str = str,
        .font = font,
        .fg = rgba_white(),
        .bg = rgba_nil(),
    };
}

void ui_label_free(struct ui_label *label)
{
    ui_str_free(&label->str);
}

void ui_label_render(
        struct ui_label *label, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_layout_add(layout, &label->w);

    rgba_render(label->bg, renderer);
    SDL_Rect rect = ui_widget_rect(&label->w);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    SDL_Point point = pos_as_point(label->w.pos);
    font_render(label->font, renderer, point, label->fg, label->str.str, label->str.len);
}
