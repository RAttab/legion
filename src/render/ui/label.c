/* label.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"


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

struct label *label_set(struct label *label, const char *str, size_t len)
{
    assert((void *)(label + 1) == (void *)label->str);
    assert(len <= label->len);
    memcpy(label->str, str, len);
}

void label_render(
        struct label *label, struct layout *layout, SDL_Renderer *renderer)
{
    layout_add(layout, &label->w);

    rgba_render(label->bg, renderer);
    sdl_err(SDL_RenderFillRect(renderer, &wdidget_rect(&label->w)));
    font_render(label->font, layout->renderer, label->fg, label->w.pos, str, len);
}
