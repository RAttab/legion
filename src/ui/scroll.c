/* scroll.c
   Rémi Attab (remi.attab@gmail.com), 12 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "scroll.h"
#include "render/ui.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_scroll_style_default(struct ui_style *s)
{
    s->scroll = (struct ui_scroll_style) {
        .fg = rgba_gray(0x88),
        .bg = s->rgba.bg,
        .width = 6,
    };
}


// -----------------------------------------------------------------------------
// scroll
// -----------------------------------------------------------------------------

struct ui_scroll ui_scroll_new(struct dim dim, struct dim cell)
{
    return (struct ui_scroll) {
        .w = ui_widget_new(dim.w, dim.h),
        .s = ui_st.scroll,
        .cell = cell,
        .rows = {0},
        .cols = {0},
        .drag = {0},
    };
}

void ui_scroll_free(struct ui_scroll *) {}


void ui_scroll_update_rows(struct ui_scroll *scroll, size_t total)
{
    scroll->rows.total = total;
    scroll->rows.first = legion_min(scroll->rows.first, scroll->rows.total);
}

void ui_scroll_update_cols(struct ui_scroll *scroll, size_t total)
{
    scroll->cols.total = total;
    scroll->cols.first = legion_min(scroll->cols.first, scroll->cols.total);
}

void ui_scroll_move_rows(struct ui_scroll *scroll, ssize_t inc)
{
    if (!scroll->rows.total) return;

    scroll->rows.first = legion_bound(
            (ssize_t) scroll->rows.first + inc,
            (ssize_t) 0,
            (ssize_t) scroll->rows.total - 1);
}

void ui_scroll_move_cols(struct ui_scroll *scroll, ssize_t inc)
{
    if (!scroll->cols.total) return;

    scroll->cols.first = legion_bound(
            (ssize_t) scroll->cols.first + inc,
            (ssize_t) 0,
            (ssize_t) scroll->cols.total - 1);
}

void ui_scroll_page_up(struct ui_scroll *scroll)
{
    ui_scroll_move_rows(scroll, -scroll->rows.visible);
}

void ui_scroll_page_down(struct ui_scroll *scroll)
{
    ui_scroll_move_rows(scroll, scroll->rows.visible);
}

size_t ui_scroll_first_row(const struct ui_scroll *scroll)
{
    return scroll->rows.first;
}

size_t ui_scroll_last_row(const struct ui_scroll *scroll)
{
    return legion_min(
            scroll->rows.total,
            scroll->rows.first + scroll->rows.visible);
}

size_t ui_scroll_first_col(const struct ui_scroll *scroll)
{
    return scroll->cols.first;
}

size_t ui_scroll_last_col(const struct ui_scroll *scroll)
{
    return legion_min(
            scroll->cols.total,
            scroll->cols.first + scroll->cols.visible);
}

void ui_scroll_visible(struct ui_scroll *scroll, size_t row, size_t col)
{
    if (scroll->rows.total) {
        row = legion_min(row, scroll->rows.total);
        if (row < scroll->rows.first) scroll->rows.first = row;
        if (row >= scroll->rows.first + scroll->rows.visible - 1)
            scroll->rows.first = row - scroll->rows.visible + 1;
    }
    else scroll->rows.first = 0;

    if (scroll->cols.total) {
        col = legion_min(col, scroll->cols.total);
        if (col < scroll->cols.first) scroll->cols.first = col;
        if (col >= scroll->cols.first + scroll->cols.visible - 1)
            scroll->cols.first = col - scroll->cols.visible + 1;
    }
    else scroll->cols.first = 0;
}

void ui_scroll_center(struct ui_scroll *scroll, size_t row, size_t col)
{
    // If we haven't displayed our first frame then visible isn't calculated
    // yet and so we need to defer our centering.
    if (!scroll->rows.visible || !scroll->cols.visible) {
        scroll->center = (struct rowcol) { .row = row, .col = col };
        return;
    }

    row = legion_min(row, scroll->rows.total);
    scroll->rows.first = row - legion_min(row, scroll->rows.visible / 2);

    col = legion_min(col, scroll->cols.total);
    if (col >= scroll->cols.visible)
        scroll->cols.first = col - legion_min(col, scroll->cols.visible / 2);
    else scroll->cols.first = 0;
}


// -----------------------------------------------------------------------------
// event / render
// -----------------------------------------------------------------------------

static SDL_Rect ui_scroll_rect_rows(struct ui_scroll *scroll)
{
    if (!scroll->rows.show) return (SDL_Rect) {0};

    int16_t height = scroll->w.dim.h;
    if (scroll->cols.total) height -= scroll->cell.h;

    SDL_Rect rect = {
        .x = scroll->w.pos.x + (scroll->w.dim.w - scroll->cell.w),
        .y = scroll->w.pos.y + ((height * scroll->rows.first) / scroll->rows.total),
        .w = scroll->s.width,
        .h = (height * scroll->rows.visible) / scroll->rows.total,
    };

    rect.x += (scroll->cell.w / 2) - (rect.w / 2);

    const int max = scroll->w.pos.y + height;
    if (rect.y + rect.h > max) rect.h = max - rect.y;

    return rect;
}

static SDL_Rect ui_scroll_rect_cols(struct ui_scroll *scroll)
{
    if (!scroll->cols.show) return (SDL_Rect) {0};

    int16_t width = scroll->w.dim.w;
    if (scroll->rows.total) width -= scroll->cell.w;

    SDL_Rect rect = {
        .x = scroll->w.pos.x + ((width * scroll->cols.first) / scroll->cols.total),
        .y = scroll->w.pos.y + (scroll->w.dim.h - scroll->cell.h),
        .w = (width * scroll->cols.visible) / scroll->cols.total,
        .h = scroll->s.width,
    };

    rect.y += (scroll->cell.h / 2) - (rect.h / 2);

    const int max = scroll->w.pos.x + width;
    if (rect.x + rect.w > max) rect.w = max - rect.x;

    return rect;
}


enum ui_ret ui_scroll_event(struct ui_scroll *scroll, const SDL_Event *ev)
{
    switch (ev->type) {

    case SDL_MOUSEWHEEL: {
        SDL_Rect widget = ui_widget_rect(&scroll->w);
        if (!ui_cursor_in(&widget)) return ui_nil;

        ui_scroll_move_rows(scroll, -ev->wheel.y);
        ui_scroll_move_cols(scroll, ev->wheel.x);
        return ui_consume;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point cursor = ui_cursor_point();

        if (scroll->rows.show) {
            SDL_Rect rows = ui_scroll_rect_rows(scroll);
            if (SDL_PointInRect(&cursor, &rows)) {
                scroll->drag.type = ui_scroll_rows;
                scroll->drag.start = cursor.y;
                scroll->drag.bar = rows.y;
                return ui_consume;
            }
        }

        if (scroll->cols.show) {
            SDL_Rect cols = ui_scroll_rect_cols(scroll);
            if (SDL_PointInRect(&cursor, &cols)) {
                scroll->drag.type = ui_scroll_cols;
                scroll->drag.start = cursor.x;
                scroll->drag.bar = cols.x;
                return ui_consume;
            }
        }

        return ui_nil;
    }

    case SDL_MOUSEBUTTONUP: {
        if (!scroll->drag.type) return ui_nil;
        memset(&scroll->drag, 0, sizeof(scroll->drag));
        return ui_consume;
    }

    case SDL_MOUSEMOTION: {
        switch (scroll->drag.type)
        {
        case ui_scroll_nil: { return ui_nil; }

        case ui_scroll_rows: {
            int16_t delta = ui_cursor_point().y - scroll->drag.start;
            int16_t bar = scroll->drag.bar + delta;
            if (bar < scroll->w.pos.y) scroll->rows.first = 0;
            else {
                size_t first = ((bar - scroll->w.pos.y) * scroll->rows.total) / scroll->w.dim.h;
                scroll->rows.first = legion_min(scroll->rows.total, first);
            }
            return ui_nil;
        }

        case ui_scroll_cols: {
            int16_t delta = ui_cursor_point().x - scroll->drag.start;
            int16_t bar = scroll->drag.bar + delta;
            if (bar < scroll->w.pos.x) scroll->cols.first = 0;
            else {
                size_t first = ((bar - scroll->w.pos.x) * scroll->cols.total) / scroll->w.dim.h;
                scroll->cols.first = legion_min(scroll->cols.total, first);
            }
            return ui_nil;
        }

        default: { assert(false); }
        }
    }

    default: { return ui_nil; }
    }
}

struct ui_layout ui_scroll_render(
        struct ui_scroll *scroll, struct ui_layout *layout, SDL_Renderer *renderer)
{
    ui_layout_add(layout, &scroll->w);

    scroll->rows.visible = scroll->w.dim.h / scroll->cell.h;
    scroll->cols.visible = scroll->w.dim.w / scroll->cell.w;

    if (scroll->center.row || scroll->center.col) {
        ui_scroll_center(scroll, scroll->center.row, scroll->center.col);
        scroll->center = (struct rowcol) { 0 };
    }

    scroll->rows.show =
        scroll->rows.first ||
        scroll->rows.visible < scroll->rows.total;
    scroll->cols.show =
        scroll->cols.first ||
        scroll->cols.visible < scroll->cols.total;

    if (scroll->cols.show) scroll->rows.visible--;
    if (scroll->rows.show) scroll->cols.visible--;

    if (!rgba_is_nil(scroll->s.bg)) {
        rgba_render(scroll->s.bg, renderer);
        SDL_Rect rect = ui_widget_rect(&scroll->w);
        sdl_err(SDL_RenderFillRect(renderer, &rect));
    }

    if (scroll->rows.show) {
        rgba_render(scroll->s.fg, renderer);
        SDL_Rect rect = ui_scroll_rect_rows(scroll);
        sdl_err(SDL_RenderFillRect(renderer, &rect));
    }

    if (scroll->cols.show) {
        rgba_render(scroll->s.fg, renderer);
        SDL_Rect rect = ui_scroll_rect_cols(scroll);
        sdl_err(SDL_RenderFillRect(renderer, &rect));
    }

    struct dim dim = make_dim(
            scroll->w.dim.w - (scroll->rows.show ? scroll->cell.w : 0),
            scroll->w.dim.h - (scroll->cols.show ? scroll->cell.h : 0));
    return ui_layout_new(scroll->w.pos, dim);
}
