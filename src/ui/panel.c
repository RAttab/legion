/* panel.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"
#include "render/font.h"

// -----------------------------------------------------------------------------
// panel
// -----------------------------------------------------------------------------

static const struct dim panel_margin = { .w = 2, .h = 2 };

static struct panel *panel_alloc(struct pos pos, struct dim dim)
{
    struct panel *panel = calloc(1, sizeof(*panel));

    panel->w = (struct widget) { .pos = pos, .dim = dim };
    panel->layout = layout_new(
            make_pos(
                    pos.x + panel_margin.w,
                    pos.y + panel_margin.h),
            make_dim(
                    dim.w - (panel_margin.w * 2),
                    dim.h - (panel_margin.h * 2)));

    panel->state = panel_visible;
    return panel;
}

struct panel *panel_slim(struct pos pos, struct dim dim)
{
    return panel_alloc(pos, dim);
}

struct panel *panel_const(struct pos pos, struct dim dim, const char *str)
{
    struct panel *panel = panel_alloc(pos, dim);

    struct font *font = font_mono6;
    panel->title = label_const(font, str);
    panel->close = button_const(font, "X");

    return panel;
}

struct panel *panel_var(struct pos pos, struct dim dim, size_t len)
{
    struct panel *panel = panel_alloc(pos, dim);

    struct font *font = font_mono6;
    panel->title = label_var(font, len);
    panel->close = button_const(font, "X");

    return panel;
}

void panel_free(struct panel *panel)
{
    label_free(panel->title);
    button_free(panel->close);
    free(panel);
}

enum ui_ret panel_event(struct panel *panel, const SDL_Event *ev)
{
    if (panel->state == panel_hidden) return ui_skip;

    switch (ev->type) {

    case SDL_KEYUP:
    case SDL_KEYDOWN: { return panel->state == panel_focused ? ui_skip : ui_nil; }

    case SDL_MOUSEWHEEL:
    case SDL_MOUSEBUTTONDOWN: {
        struct SDL_Rect rect = widget_rect(&panel->w);
        panel->state = sdl_rect_contains(&rect, &core.cursor.point) ?
            panel_focused : panel_visible;
        if (panel->state != panel_focused) return ui_skip;
        // fallthrough
    }
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEMOTION: {
        if (panel->close && button_event(panel->close, ev) == ui_consume) {
            if (panel->close->state == button_pressed) panel->state = panel_hidden;
            return ui_consume;
        }
        return ui_nil;
    }

    default: {
        if (ev->type == core.event) {
            if (ev->user.code == EV_FOCUS) {
                struct panel *target = (void *) ev->user.data1;
                panel->state = target == panel ? panel_focused : panel_visible;
            }
        }
        return ui_nil;
    }

    }
}

struct layout panel_render(struct panel *panel, SDL_Renderer *renderer)
{
    if (panel->state == panel_hidden) return (struct layout) {0};

    struct SDL_Rect rect = widget_rect(&panel->w);

    rgba_render(rgba_gray_a(0x11, 0x88), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    if (panel->title || panel->close) {
        rgba_render(rgba_gray(0x22), renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = rect.x, .y = rect.y,
                            .w = rect.w,
                            .h = panel->close->w.dim.h + panel_margin.h }));

        rgba_render(rgba_gray(0x22), renderer);
        sdl_err(SDL_RenderDrawRect(renderer, &rect));
    }

    struct layout layout = panel->layout;

    if (panel->title) {
        panel->title->fg = panel->state == panel_focused ? rgba_white() : rgba_gray(88);
        label_render(panel->title, &layout, renderer);
    }

    if (panel->close) {
        layout_right(&layout, &panel->close->w);
        button_render(panel->close, &layout, renderer);
    }

    if (panel->title || panel->close) {
        layout_next_row(&layout);
        layout_sep_y(&layout, 4);
    }

    return layout;
}
