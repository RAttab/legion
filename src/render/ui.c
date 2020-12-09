/* ui.c
   Rémi Attab (remi.attab@gmail.com), 07 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "ui.h"

#include "utils/sdl.h"

// -----------------------------------------------------------------------------
// ui_toggle
// -----------------------------------------------------------------------------

void ui_toggle_size(struct font *font, size_t str_len, int *width, int *height)
{
    if (width) *width = font->glyph_w * (str_len + 2);
    if (height) *height = font->glyph_h;
}

void ui_toggle_init(
        struct ui_toggle *toggle,
        const struct SDL_Rect *rect,
        const char * str, size_t str_len)
{
    toggle->rect = *rect;

    assert(str_len < sizeof(toggle->str) - 1);
    memcpy(toggle->str, str, str_len);
    toggle->str[str_len] = 0;
    toggle->str_len = str_len;
}

void ui_toggle_render(
        struct ui_toggle *toggle,
        SDL_Renderer *renderer,
        SDL_Point pos,
        struct font *font)
{
    font_reset(font);

    if (toggle->hover) {
        const uint8_t gray = 0x18;
        sdl_err(SDL_SetRenderDrawColor(renderer, gray, gray, gray, SDL_ALPHA_OPAQUE));
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = pos.x, .y = pos.y,
                            .w = toggle->rect.w, .h = toggle->rect.h}));
    }

    size_t font_w = font->glyph_w;
    const char *prefix = toggle->selected ? "- " : "+ ";
    font_render(font, renderer, prefix, 2, pos);

    pos.x += font_w * 2;
    font_render(font, renderer, toggle->str, toggle->str_len, pos);
}

enum ui_toggle_ret ui_toggle_events(struct ui_toggle *toggle, SDL_Event *event)
{
    switch (event->type) {

    case SDL_MOUSEMOTION: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&toggle->rect, &point)) {
            if (toggle->hover) {
                toggle->hover = false;
                return ui_toggle_invalidate;
            }
            return ui_toggle_nil;
        }

        if (!toggle->hover) {
            toggle->hover = true;
            return ui_toggle_invalidate;
        }
        return ui_toggle_nil;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point point = core.cursor.point;
        if (!sdl_rect_contains(&toggle->rect, &point)) return ui_toggle_nil;

        toggle->selected = !toggle->selected;
        return ui_toggle_flip | ui_toggle_consume | ui_toggle_invalidate;
    }

    default: { return ui_toggle_nil; }
    }

}
