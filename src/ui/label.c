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

struct label *label_const(struct font *font, const char *str)
{
    size_t len = strnlen(str, label_cap);
    struct label *label = calloc(1, sizeof(*label));

    *label = (struct label) {
        .w = make_widget(len * font->glyph_w, font->glyph_h),
        .font = font,
        .fg = rgba_white(),
        .bg = rgba_nil(),
        .len = len,
        .str = str,
    };
    return label;
}

struct label *label_var(struct font *font, size_t len)
{
    assert(len <= label_cap);

    struct label *label = calloc(1, sizeof(*label) + len);
    *label = (struct label) {
        .w = make_widget(len * font->glyph_w, font->glyph_h),
        .font = font,
        .fg = rgba_white(),
        .bg = rgba_nil(),
        .len = len,
        .str = (void *)(label + 1),
    };
    return label;
}

void label_free(struct label *label)
{
    free(label);
}

void label_set(struct label *label, const char *str, size_t len)
{
    assert((void *)(label + 1) == (void *)label->str);
    assert(len <= label->len);
    memcpy(label + 1, str, len);
}

void label_setf(struct label *label, const char *fmt, ...)
{
    assert((void *)(label + 1) == (void *)label->str);

    va_list args;
    va_start(args, fmt);
    ssize_t n = vsnprintf((void *)(label + 1), label->len, fmt, args);
    va_end(args);

    assert(n >= 0);
}

void label_render(
        struct label *label, struct layout *layout, SDL_Renderer *renderer)
{
    layout_add(layout, &label->w);

    rgba_render(label->bg, renderer);
    SDL_Rect rect = widget_rect(&label->w);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    size_t len = strnlen(label->str, label->len);
    SDL_Point point = pos_as_point(label->w.pos);
    font_render(label->font, renderer, point, label->fg, label->str, len);
}
