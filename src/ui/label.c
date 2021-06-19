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

struct ui_label ui_label_const(struct font *font, const char *str)
{
    size_t len = strnlen(str, ui_label_cap);
    return (struct ui_label) {
        .w = ui_widget_new(len * font->glyph_w, font->glyph_h),
        .font = font,
        .fg = rgba_white(),
        .bg = rgba_nil(),
        .len = len,
        .cap = 0,
        .str = str,
    };
}

struct ui_label ui_label_var(struct font *font, size_t len)
{
    assert(len <= ui_label_cap);
    return (struct ui_label) {
        .w = ui_widget_new(len * font->glyph_w, font->glyph_h),
        .font = font,
        .fg = rgba_white(),
        .bg = rgba_nil(),
        .len = 0,
        .cap = len,
        .str = calloc(len, sizeof(char)),
    };
}

void ui_label_free(struct ui_label *label)
{
    if (label->cap) free((char *) label->str);
}

void ui_label_set(struct ui_label *label, const char *str, size_t len)
{
    if (!label->cap) {
        label->str = str;
        label->len = len;
        return;
    }

    assert(len <= label->cap);
    memcpy((char *)label->str, str, len);
    label->len = len;
}

void ui_label_setf(struct ui_label *label, const char *fmt, ...)
{
    assert(label->cap);

    va_list args;
    va_start(args, fmt);
    ssize_t len = vsnprintf((char *)(label->str), label->cap, fmt, args);
    va_end(args);

    assert(len >= 0);
    label->len = len;
}

void ui_label_render(
        struct ui_label *label, struct ui_layout *layout, SDL_Renderer *renderer)
{
    assert(label->str);
    ui_layout_add(layout, &label->w);

    rgba_render(label->bg, renderer);
    SDL_Rect rect = ui_widget_rect(&label->w);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    size_t len = strnlen(label->str, label->len);
    SDL_Point point = pos_as_point(label->w.pos);
    font_render(label->font, renderer, point, label->fg, label->str, len);
}
