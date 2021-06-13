/* panel.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"

// -----------------------------------------------------------------------------
// panel
// -----------------------------------------------------------------------------

static struct font *panel_font = font_mono6;
static struct dim panel_margin = { .w = 2, .h = 2 };

static panel *panel_alloc(struct pos pos, struct dim dim)
{
    struct panel *panel = calloc(1, sizeof(*panel));

    panel->w = (struct widget) { .pos = pos, .dim = dim };
    panel->layout = make_layout(
            make_pos(
                    pos.x + panel_margin.w,
                    pos.y + panel_margin.h),
            make_dim(
                    dim.w - (panel_margin.w * 2),
                    dim.h - (panel_margin.h * 2)));

    panel->state = panel_visible;

    struct font *font = font_mono4;
    panel->close = button_const(font, "X");
    panel->title = label_var(font, (panel->w.dim.w / font->glyph_w) - 3);
    return panel;
}

struct panel *panel_const(struct pos pos, struct dim dim, const char *str)
{
    struct panel *panel = panel_alloc(pos, dim);
    panel->title = label_const(str);
    return panel;
}

struct panel *panel_var(struct pos pos, struct dim pos, size_t len)
{
    struct panel *panel = panel_alloc(pos, dim);
    panel->title = label_var(len);
    return panel;
}

struct panel *panel_set(struct panel *panel, const char *str, size_t len)
{
    label_set(panel->title, str, len);
}

enum ui_ret panel_event(struct panel *panel, const SDL_Event *ev)
{
    if (panel->state == panel_hidden) return ui_skip;

    switch (ev->type) {

    case SDL_KEYUP:
    case SDL_KEYDOWN: { return !panel->focus ? ui_skip : ui_nil; }

    case SDL_MOUSEWHEEL:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN: {
        struct SDL_Rect rect = widget_rect(&button->w);
        panel->focus = sdl_rect_contains(&rect, &core.cursor.point);
        if (!panel->focus) return ui_skip;
        // fallthrough
    }
    case SDL_MOUSEMOTION: {
        if (button_event(panel->close, ev) == ui_consume) {
            if (panel->close->state == button_pressed) panel->state = panel_hidden;
            return ui_consume;
        }
        return ui_nil;
    }

    default: { return ui_nil; }
    }
}

struct layout panel_render(struct panel *panel, SDL_Renderer *renderer)
{
    if (panel->state == panel_hidden) return (struct layout) {0};

    struct SDL_Rect rect = widget_rect(&button->w);
    sdl_err(SDL_RenderSetClipRect(renderer, &rect));

    rgba_render(make_gray_a(0x11, 0x88), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    rgba_render(make_gray(0x22), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                        .x = rect.x, .y = rect.y,
                        .w = rect.w, .h = panel->title->font->glyph_h }));

    rgba_render(make_gray(0x22), renderer);
    sdl_err(SDL_RenderDrawRect(renderer, &rect));

    struct layout layout = panel->layout;
    button_render(panel->close, &layout, renderer);
    layout_ysep(&layout, 4);

    panel->title->fg = panel->focus ? rgba_white() : rgba_gray(88);
    label_render(panel->title, &layout, renderer);

    layout_next_row(&layout);
    layout_sep_y(&layout, 4);

    return layout;
}
