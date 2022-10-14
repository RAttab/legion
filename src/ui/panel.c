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

static struct ui_panel *ui_panel_curr = NULL;
struct ui_panel *ui_panel_current(void) { return ui_panel_curr; }


static struct ui_panel *ui_panel_new(struct pos pos, struct dim dim)
{
    struct ui_panel_style *s = &ui_st.panel;

    struct ui_panel *panel = calloc(1, sizeof(*panel));
    *panel = (struct ui_panel) {
        .w = (struct ui_widget) { .pos = pos, .dim = dim },
        .s = *s,

        .state = ui_panel_visible,
        .layout = ui_layout_new(
                make_pos(
                        pos.x + s->margin.w,
                        pos.y + s->margin.h),
                make_dim(
                        dim.w - (s->margin.w * 2),
                        dim.h - (s->margin.h * 2))),
    };

    ui_panel_curr = panel;
    return panel;
}

struct ui_panel *ui_panel_menu(struct pos pos, struct dim dim)
{
    struct ui_panel *panel = ui_panel_new(pos, dim);
    panel->menu = true;
    return panel;
}

struct ui_panel *ui_panel_title(struct pos pos, struct dim dim, struct ui_str str)
{
    struct ui_panel *panel = ui_panel_new(pos, dim);
    panel->title = ui_label_new(str);
    panel->close = ui_button_new(ui_str_c("X"));
    return panel;
}

void ui_panel_free(struct ui_panel *panel)
{
    if (!panel->menu) {
        ui_label_free(&panel->title);
        ui_button_free(&panel->close);
    }
    free(panel);
}

void ui_panel_resize(struct ui_panel *panel, struct dim dim)
{
    panel->w.dim = dim;
    ui_layout_resize(&panel->layout, panel->layout.base.pos, make_dim(
                    dim.w - (panel->s.margin.w * 2),
                    dim.h - (panel->s.margin.h * 2)));
}

void ui_panel_show(struct ui_panel *panel)
{
    panel->state = ui_panel_focused;
    render_push_event(EV_FOCUS_PANEL, (uintptr_t) panel, 0);
}

void ui_panel_hide(struct ui_panel *panel)
{
    panel->state = ui_panel_hidden;
}

bool ui_panel_is_visible(struct ui_panel *panel)
{
    return panel->state != ui_panel_hidden;
}


enum ui_ret ui_panel_event(struct ui_panel *panel, const SDL_Event *ev)
{
    if (panel->state == ui_panel_hidden) return ui_skip;

    switch (ev->type) {

    case SDL_KEYUP:
    case SDL_KEYDOWN: { return panel->state == ui_panel_focused ? ui_nil : ui_skip; }

    case SDL_MOUSEWHEEL:
    case SDL_MOUSEBUTTONUP: {
        struct SDL_Rect rect = ui_widget_rect(&panel->w);
        if (!sdl_rect_contains(&rect, &render.cursor.point)) {
            panel->state = ui_panel_visible;
            return ui_skip;
        }

        if (panel->state == ui_panel_visible) {
            panel->state = ui_panel_focused;
            render_push_event(EV_FOCUS_PANEL, (uintptr_t) panel, 0);
        }

        break;
    }

    case SDL_MOUSEBUTTONDOWN: {
        struct SDL_Rect rect = ui_widget_rect(&panel->w);
        if (!sdl_rect_contains(&rect, &render.cursor.point))
            return ui_skip;
        break;
    }

    default: { break; }
    }

    if (ev->type == render.event) {
        if (ev->user.code == EV_FOCUS_PANEL) {
            struct ui_panel *target = (void *) ev->user.data1;
            panel->state = target == panel ? ui_panel_focused : ui_panel_visible;
        }
    }

    enum ui_ret ret = 0;
    if (!panel->menu && (ret = ui_button_event(&panel->close, ev))) {
        if (ret == ui_action) panel->state = ui_panel_hidden;
        return ret;
    }

    return ui_nil;
}

enum ui_ret ui_panel_event_consume(struct ui_panel *panel, const SDL_Event *ev)
{
    if (panel->state == ui_panel_hidden) return ui_skip;

    switch (ev->type) {

    case SDL_KEYUP:
    case SDL_KEYDOWN: {
        return panel->state == ui_panel_focused ? ui_consume : ui_nil;
    }

    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN: {
        struct SDL_Rect rect = ui_widget_rect(&panel->w);
        return sdl_rect_contains(&rect, &render.cursor.point) ? ui_consume : ui_nil;
    }

    default: { return ui_nil; }
    }
}

struct ui_layout ui_panel_render(struct ui_panel *panel, SDL_Renderer *renderer)
{
    if (panel->state == ui_panel_hidden) return (struct ui_layout) {0};

    struct SDL_Rect rect = ui_widget_rect(&panel->w);

    rgba_render(panel->s.bg, renderer);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    if (!panel->menu) {
        struct rgba bg = panel->state == ui_panel_focused ?
            panel->s.focused.bg : panel->s.head.bg;

        rgba_render(bg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = rect.x, .y = rect.y,
                            .w = rect.w,
                            .h = panel->close.w.dim.h + panel->s.margin.h }));

        rgba_render(panel->s.border, renderer);
        sdl_err(SDL_RenderDrawRect(renderer, &rect));
    }

    struct ui_layout layout = panel->layout;

    if (!panel->menu) {
        if (panel->state == ui_panel_focused) {
            panel->title.s.font = panel->s.focused.font;
            panel->title.s.fg = panel->s.focused.fg;
        }
        else {
            panel->title.s.font = panel->s.head.font;
            panel->title.s.fg = panel->s.head.fg;
        }
        ui_label_render(&panel->title, &layout, renderer);

        ui_layout_dir(&layout, ui_layout_left);
        ui_button_render(&panel->close, &layout, renderer);

        ui_layout_next_row(&layout);
        ui_layout_sep_y(&layout, 4);
        ui_layout_dir(&layout, ui_layout_right);
    }

    return layout;
}
