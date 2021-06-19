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

static const struct dim ui_panel_margin = { .w = 2, .h = 2 };

static struct ui_panel *ui_panel_alloc(struct pos pos, struct dim dim)
{
    struct ui_panel *panel = calloc(1, sizeof(*panel));

    panel->w = (struct ui_widget) { .pos = pos, .dim = dim };
    panel->layout = ui_layout_new(
            make_pos(
                    pos.x + ui_panel_margin.w,
                    pos.y + ui_panel_margin.h),
            make_dim(
                    dim.w - (ui_panel_margin.w * 2),
                    dim.h - (ui_panel_margin.h * 2)));

    panel->state = ui_panel_visible;
    return panel;
}

struct ui_panel *ui_panel_slim(struct pos pos, struct dim dim)
{
    return ui_panel_alloc(pos, dim);
}

struct ui_panel *ui_panel_const(struct pos pos, struct dim dim, const char *str)
{
    struct ui_panel *panel = ui_panel_alloc(pos, dim);

    struct font *font = font_mono6;
    panel->title = ui_label_const(font, str);
    panel->close = ui_button_const(font, "X");

    return panel;
}

struct ui_panel *ui_panel_var(struct pos pos, struct dim dim, size_t len)
{
    struct ui_panel *panel = ui_panel_alloc(pos, dim);

    struct font *font = font_mono6;
    panel->title = ui_label_var(font, len);
    panel->close = ui_button_const(font, "X");

    return panel;
}

void ui_panel_free(struct ui_panel *panel)
{
    ui_label_free(panel->title);
    ui_button_free(panel->close);
    free(panel);
}

enum ui_ret ui_panel_event(struct ui_panel *panel, const SDL_Event *ev)
{
    if (panel->state == ui_panel_hidden) return ui_skip;

    switch (ev->type) {

    case SDL_KEYUP:
    case SDL_KEYDOWN: { return panel->state == ui_panel_focused ? ui_nil : ui_skip; }

    case SDL_MOUSEWHEEL:
    case SDL_MOUSEBUTTONDOWN: {
        struct SDL_Rect rect = ui_widget_rect(&panel->w);
        panel->state = sdl_rect_contains(&rect, &core.cursor.point) ?
            ui_panel_focused : ui_panel_visible;
        if (panel->state != ui_panel_focused) return ui_skip;
        // fallthrough
    }
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEMOTION: {
        if (panel->close && ui_button_event(panel->close, ev) == ui_consume) {
            if (panel->close->state == ui_button_pressed) panel->state = ui_panel_hidden;
            return ui_consume;
        }
        return ui_nil;
    }

    default: {
        if (ev->type == core.event) {
            if (ev->user.code == EV_FOCUS) {
                struct ui_panel *target = (void *) ev->user.data1;
                panel->state = target == panel ? ui_panel_focused : ui_panel_visible;
            }
        }
        return ui_nil;
    }

    }
}

struct ui_layout ui_panel_render(struct ui_panel *panel, SDL_Renderer *renderer)
{
    if (panel->state == ui_panel_hidden) return (struct ui_layout) {0};

    struct SDL_Rect rect = ui_widget_rect(&panel->w);

    rgba_render(rgba_gray_a(0x11, 0x88), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    if (panel->title || panel->close) {
        rgba_render(rgba_gray(0x22), renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = rect.x, .y = rect.y,
                            .w = rect.w,
                            .h = panel->close->w.dim.h + ui_panel_margin.h }));

        rgba_render(rgba_gray(0x22), renderer);
        sdl_err(SDL_RenderDrawRect(renderer, &rect));
    }

    struct ui_layout layout = panel->layout;

    if (panel->title) {
        panel->title->fg = panel->state == ui_panel_focused ? rgba_white() : rgba_gray(0xAA);
        ui_label_render(panel->title, &layout, renderer);
    }

    if (panel->close) {
        ui_layout_right(&layout, &panel->close->w);
        ui_button_render(panel->close, &layout, renderer);
    }

    if (panel->title || panel->close) {
        ui_layout_next_row(&layout);
        ui_layout_sep_y(&layout, 4);
    }

    return layout;
}
