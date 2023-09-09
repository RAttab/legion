/* tabs.c
   Rémi Attab (remi.attab@gmail.com), 04 Sep 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "tabs.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_tabs_style_default(struct ui_style *s)
{
    s->tabs = (struct ui_tabs_style) {
        .pad = s->pad.box,

        .font = s->font.base,
        .bold = s->font.bold,

        .fg = s->rgba.fg,
        .line = s->rgba.box.border,
        .hover = rgba_gray(0x33),
        .pressed = rgba_gray(0x22),
    };
}



// -----------------------------------------------------------------------------
// tabs
// -----------------------------------------------------------------------------

struct ui_tabs ui_tabs_new(size_t chars, bool close)
{
    const struct ui_tabs_style *s = &ui_st.tabs;

    return (struct ui_tabs) {
        .w = ui_widget_new(ui_layout_inf, s->font->glyph_h + (s->pad.h * 2)),
        .s = *s,

        .str = ui_str_v(chars),
        .close = { .show = close },
    };
}

void ui_tabs_free(struct ui_tabs *ui)
{
    for (size_t i = 0; i < ui->cap; ++i) {
        struct ui_tab *tab = ui->list + i;
        if (tab->str.cap) ui_str_free(&tab->str);
    }

    ui_str_free(&ui->str);
    free(ui->list);
}


void ui_tabs_clear(struct ui_tabs *ui)
{
    ui->select.user = 0;
    ui->select.update = false;
}

void ui_tabs_select(struct ui_tabs *ui, uint64_t user)
{
    if (ui->select.user == user) return;

    ui->select.user = user;
    ui->select.update = true;
}

uint64_t ui_tabs_selected(struct ui_tabs *ui)
{
    return ui->select.user;
}

uint64_t ui_tabs_closed(struct ui_tabs *ui)
{
    return legion_xchg(&ui->close.user, 0UL);
}

void ui_tabs_reset(struct ui_tabs *ui)
{
    ui->len = 0;
}

struct ui_str *ui_tabs_add_s(struct ui_tabs *ui, uint64_t user, struct rgba fg)
{
    assert(user);
    ui->update = true;

    if (ui->len == ui->cap) {
        size_t old = legion_xchg(&ui->cap, ui->cap ? ui->cap * 2 : 8);
        ui->list = realloc_zero(ui->list, old, ui->cap, sizeof(*ui->list));
    }

    struct ui_tab *tab = ui->list + ui->len;
    ui->len++;

    tab->fg = fg;
    tab->user = user;

    if (!tab->str.cap) tab->str = ui_str_clone(&ui->str);
    return &tab->str;
}

struct ui_str *ui_tabs_add(struct ui_tabs *ui, uint64_t user)
{
    return ui_tabs_add_s(ui, user, ui->s.fg);
}

static void ui_tabs_update(struct ui_tabs *ui)
{
    if (!ui->update && !ui->select.update) return;

    int16_t x = ui->w.pos.x;
    int16_t w = ui->w.dim.w;
    int16_t cell = ui->s.font->glyph_w;
    ui->first = legion_min(ui->first, ui->len - 1);

    {
        ui->left.w = ui->s.pad.w * 2 + 1 * cell;
        ui->left.x = x;
        ui->right.w = ui->left.w;
        ui->right.x = x + w - ui->right.w;

        x += ui->left.w;
        w -= ui->left.w + ui->right.w;
    }

    {
        int32_t sum = 0;
        for (size_t i = 0; i < ui->len; ++i) {
            struct ui_tab *tab = ui->list + i;
            tab->w =
                (tab->str.len * cell) +
                (ui->close.show ? 2 * cell : 0) +
                ui->s.pad.w * 2;
            sum += tab->w;
        }

        if (sum < w) ui->first = 0;
    }

    if (ui->select.update) {
        size_t ix = 0;
        while (ix < ui->len && ui->list[ix].user != ui->select.user) ++ix;
        assert(ui->select.user);
        assert(ix < ui->len);

        int32_t sum = 0;
        ui->first = legion_min(ui->first, ix);
        for (; ix > ui->first; --ix) {
            struct ui_tab *tab = ui->list + ix;
            if (sum + tab->w > w) break;
            sum += tab->w;
        }
        ui->first = ix;
        ui->select.update = false;
    }

    {
        int32_t sum = 0;
        for (size_t i = ui->first; i < ui->len; ++i) {
            struct ui_tab *tab = ui->list + i;
            if (sum + tab->w > w) break;
            sum += tab->w;
        }

        for (; ui->first; --ui->first) {
            struct ui_tab *prev = ui->list + (ui->first - 1);
            if (sum + prev->w > w) break;
            sum += prev->w;
        }

    }

    ui->left.show = ui->first > 0;
    ui->right.show = false;

    for (size_t i = 0; i < ui->len; ++i) {
        struct ui_tab *tab = ui->list + i;

        tab->hidden =
            i < ui->first ||
            ui->right.show ||
            (ui->right.show = tab->w > w);
        if (tab->hidden) continue;

        tab->x = x;
        x += tab->w;
        w -= tab->w;
    }

    ui->update = false;
}

enum ui_ret ui_tabs_event(struct ui_tabs *ui, const SDL_Event *ev)
{
    if (ev->type != SDL_MOUSEBUTTONUP) return ui_nil;

    SDL_Rect rect = {
        .x = 0, .y = ui->w.pos.y,
        .w = 0, .h = ui->w.dim.h,
    };

    rect.x = ui->left.x; rect.w = ui->left.w;
    if (ui->left.show && ui_cursor_in(&rect)) {
        assert(ui->first > 0);
        ui->first--;
        ui->update = true;
        return ui_consume;
    }

    rect.x = ui->right.x; rect.w = ui->right.w;
    if (ui->right.show && ui_cursor_in(&rect)) {
        assert(ui->first > 0);
        ui->first++;
        ui->update = true;
        return ui_consume;
    }

    for (size_t i = ui->first; i < ui->len; ++i) {
        struct ui_tab *tab = ui->list + i;
        if (tab->hidden) continue;

        rect.x = tab->x; rect.w = tab->w;
        if (!ui_cursor_in(&rect)) continue;

        rect.w = ui->s.font->glyph_w;
        rect.x = (tab->x + tab->w) - (ui->s.pad.w + rect.w);

        if (!ui->close.show || !ui_cursor_in(&rect)) {
            ui->select.user = tab->user;
            return ui_action;
        }

        ui->close.user = tab->user;
        struct ui_tab swap = *tab;
        memmove(tab, tab + 1, (ui->len - (i + 1)) * sizeof(*tab));
        *(ui->list + (ui->len - 1)) = swap;
        ui->len--;

        if (ui->close.user == ui->select.user) {
            if (i < ui->len) ui->select.user = tab->user;
            else if (i > 0) ui->select.user = (tab - 1)->user;
            else ui->select.user = 0;
        }

        ui->update = true;
        return ui_action;
    }

    return ui_nil;
}

void ui_tabs_render(
        struct ui_tabs *ui, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_layout_add(layout, &ui->w);

    ui_tabs_update(ui);

    const int16_t h = ui->w.dim.h;
    const int16_t y0 = ui->w.pos.y;
    const int16_t y1 = y0 + h - 1;

    { // left
        SDL_Rect rect = { .x = ui->left.x, .y = y0, .w = ui->left.w, .h = h };

        if (ui->left.show) {
            if (ui_cursor_in(&rect)) {
                rgba_render(ui->s.hover, renderer);
                sdl_err(SDL_RenderFillRect(renderer, &rect));
            }

            SDL_Point pos = {
                .x = rect.x + ui->s.pad.w,
                .y = rect.y + ui->s.pad.h,
            };
            font_render(ui->s.font, renderer, pos, ui->s.fg, "<", 1);
        }

        rgba_render(ui->s.line, renderer);
        sdl_err(SDL_RenderDrawLine(renderer,
                        ui->left.x, y1, ui->left.x + ui->left.w, y1));
    }


    int16_t x_last = 0;

    for (size_t i = ui->first; i < ui->len; ++i) {
        struct ui_tab *tab = ui->list + i;
        if (tab->hidden) break;

        SDL_Rect rect = { .x = tab->x, .y = y0, .w = tab->w, .h = h };
        if (ui_cursor_in(&rect)) {
            struct rgba bg = ui_cursor_button_down(SDL_BUTTON_LEFT) ?
                ui->s.pressed : ui->s.hover;
            rgba_render(bg, renderer);
            sdl_err(SDL_RenderFillRect(renderer, &rect));
        }

        const struct font *font = tab->user == ui->select.user ?
            ui->s.bold : ui->s.font;

        SDL_Point pos = {
            .x = rect.x + ui->s.pad.w,
            .y = rect.y + ui->s.pad.h,
        };
        font_render(font, renderer, pos, tab->fg, tab->str.str, tab->str.len);

        if (ui->close.show) {
            pos.x = (rect.x + rect.w) - (ui->s.pad.w + font->glyph_w);
            font_render(ui->s.font, renderer, pos, ui->s.fg, "x", 1);
        }

        rgba_render(ui->s.line, renderer);
        if (tab->user != ui->select.user) {
            sdl_err(SDL_RenderDrawRect(renderer, &rect));
            continue;
        }

        const int16_t x0 = tab->x;
        const int16_t x1 = x0 + tab->w;
        sdl_err(SDL_RenderDrawLine(renderer, x0, y0, x0, y1));
        sdl_err(SDL_RenderDrawLine(renderer, x0, y0, x1, y0));
        sdl_err(SDL_RenderDrawLine(renderer, x1, y0, x1, y1));

        x_last = x1;
    }

    { // right
        SDL_Rect rect = { .x = ui->right.x, .y = y0, .w = ui->right.w, .h = h };
        if (ui->left.show) {
            if (ui_cursor_in(&rect)) {
                rgba_render(ui->s.hover, renderer);
                sdl_err(SDL_RenderFillRect(renderer, &rect));
            }

            SDL_Point pos = {
                .x = rect.x + ui->s.pad.w,
                .y = rect.y + ui->s.pad.h,
            };
            font_render(ui->s.font, renderer, pos, ui->s.fg, ">", 1);
        }

        const int16_t x1 = ui->w.pos.x + ui->w.dim.w;
        rgba_render(ui->s.line, renderer);
        sdl_err(SDL_RenderDrawLine(renderer, x_last, y1, x1, y1));
    }
}
