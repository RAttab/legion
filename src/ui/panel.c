/* panel.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "panel.h"
#include "focus.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_panel_style_default(struct ui_style *s)
{
    s->panel = (struct ui_panel_style) {
        .margin = make_dim(4, 4),

        .bg = rgba_gray_a(0x11, 0xEE),
        .border = s->rgba.box.border,

        .head = {
            .font = s->font.base,
            .fg = rgba_gray(0xAA),
            .bg = rgba_gray(0x11),
        },

        .focused = {
            .font = s->font.bold,
            .fg = s->rgba.fg,
            .bg = rgba_gray(0x22),
        },
    };
}


// -----------------------------------------------------------------------------
// panel
// -----------------------------------------------------------------------------

static struct ui_panel *ui_panel_curr = NULL;
struct ui_panel *ui_panel_current(void) { return ui_panel_curr; }

static struct ui_panel *ui_panel_new(struct dim dim)
{
    struct ui_panel_style *s = &ui_st.panel;

    struct ui_panel *panel = calloc(1, sizeof(*panel));
    *panel = (struct ui_panel) {
        .w = (struct ui_widget) { .pos = {0}, .dim = dim },
        .s = *s,
        .visible = true,
    };

    if (panel->w.dim.w != ui_layout_inf)
        panel->w.dim.w += panel->s.margin.w * 2;
    if (panel->w.dim.h != ui_layout_inf)
        panel->w.dim.h += panel->s.margin.h * 2;

    ui_panel_curr = panel;
    return panel;
}

struct ui_panel *ui_panel_menu(struct dim dim)
{
    struct ui_panel *panel = ui_panel_new(dim);
    panel->menu = true;
    return panel;
}

struct ui_panel *ui_panel_title(struct dim dim, struct ui_str str)
{
    struct ui_panel *panel = ui_panel_new(dim);
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
    if (dim.w != ui_layout_inf)
        panel->w.dim.w = dim.w + (panel->s.margin.w * 2);
    if (dim.h != ui_layout_inf)
        panel->w.dim.h = dim.h + (panel->s.margin.h * 2);
}


void ui_panel_show(struct ui_panel *panel)
{
    panel->visible = true;
    ui_focus_panel_acquire(panel);
}

void ui_panel_hide(struct ui_panel *panel)
{
    panel->visible = false;
    ui_focus_panel_release(panel);
}


enum ui_ret ui_panel_event(struct ui_panel *panel, const SDL_Event *ev)
{
    if (!panel->visible) return ui_skip;

    switch (ev->type) {

    case SDL_KEYUP:
    case SDL_KEYDOWN: { return ui_focus_panel() == panel ? ui_nil : ui_skip; }

    case SDL_MOUSEWHEEL:
    case SDL_MOUSEBUTTONUP: {
        struct SDL_Rect rect = ui_widget_rect(&panel->w);
        if (!ui_cursor_in(&rect)) return ui_skip;
        break;
    }

    case SDL_MOUSEBUTTONDOWN: {
        struct SDL_Rect rect = ui_widget_rect(&panel->w);
        if (!ui_cursor_in(&rect)) {
            ui_focus_panel_release(panel);
            return ui_skip;
        }

        ui_focus_panel_acquire(panel);
        break;
    }

    default: { break; }
    }

    enum ui_ret ret = 0;
    if (!panel->menu && (ret = ui_button_event(&panel->close, ev))) {
        if (ret == ui_action) ui_panel_hide(panel);
        return ret;
    }

    if (ev->type == SDL_MOUSEBUTTONDOWN && !panel->menu) {
        struct SDL_Rect rect = ui_widget_rect(&panel->w);
        rect.h = panel->close.w.dim.h + panel->s.margin.h;
        if (ui_cursor_in(&rect)) return ui_consume;
    }
    
    return ui_nil;
}

enum ui_ret ui_panel_event_consume(struct ui_panel *panel, const SDL_Event *ev)
{
    if (!panel->visible) return ui_skip;

    switch (ev->type) {

    case SDL_KEYUP:
    case SDL_KEYDOWN: { return ui_focus_panel() == panel ? ui_consume : ui_nil; }

    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN: {
        struct SDL_Rect rect = ui_widget_rect(&panel->w);
        return ui_cursor_in(&rect) ? ui_consume : ui_nil;
    }

    default: { return ui_nil; }
    }
}

struct ui_layout ui_panel_render(
        struct ui_panel *panel, struct ui_layout *layout, SDL_Renderer *renderer)
{
    if (!panel->visible) return (struct ui_layout) {0};

    ui_layout_add(layout, &panel->w);
    struct SDL_Rect rect = ui_widget_rect(&panel->w);

    rgba_render(panel->s.bg, renderer);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    const bool focused = ui_focus_panel() == panel;

    if (!panel->menu) {
        struct rgba bg = focused ? panel->s.focused.bg : panel->s.head.bg;
        rgba_render(bg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = rect.x, .y = rect.y,
                            .w = rect.w,
                            .h = panel->close.w.dim.h + panel->s.margin.h }));

        rgba_render(panel->s.border, renderer);
        sdl_err(SDL_RenderDrawRect(renderer, &rect));
    }

    struct ui_layout inner = ui_layout_new(
            make_pos(
                    panel->w.pos.x + panel->s.margin.w,
                    panel->w.pos.y + panel->s.margin.h),
            make_dim(
                    panel->w.dim.w - (panel->s.margin.w * 2),
                    panel->w.dim.h - (panel->s.margin.h * 2)));

    if (!panel->menu) {
        if (focused) {
            panel->title.s.font = panel->s.focused.font;
            panel->title.s.fg = panel->s.focused.fg;
        }
        else {
            panel->title.s.font = panel->s.head.font;
            panel->title.s.fg = panel->s.head.fg;
        }
        ui_label_render(&panel->title, &inner, renderer);

        ui_layout_dir(&inner, ui_layout_right_left);
        ui_button_render(&panel->close, &inner, renderer);

        ui_layout_next_row(&inner);
        ui_layout_sep_y(&inner, 4);
        ui_layout_dir(&inner, ui_layout_left_right);
    }

    return inner;
}
