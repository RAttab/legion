/* histo.c
   Rémi Attab (remi.attab@gmail.com), 10 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"


// -----------------------------------------------------------------------------
// histo
// -----------------------------------------------------------------------------

enum { ui_histo_val_len = 6 };

struct ui_histo ui_histo_new(
        struct dim dim,
        const struct ui_histo_series *list, size_t len)
{
    struct ui_histo_style *s = &ui_st.histo;

    struct ui_histo histo = (struct ui_histo) {
        .w = ui_widget_new(dim.w, dim.h),
        .s = *s,

        .v = { .bound = 1 },
        .t = { .scale = 1 },

        .edge = { .t = 0, .row = 0 },
        .hover = { .v = 0, .row = 0, .active = false },

        .series = {
            .len = len, .cols = 0, .rows = 0,
            .list = calloc(len, sizeof(*list)),
        },
    };

    memcpy(histo.series.list, list, len * sizeof(*list));

    for (size_t i = 0; i < len; ++i) {
        assert(!i || list[i-1].col <= list[i].col);
        histo.series.cols = legion_max(histo.series.cols, list[i].col + 1LU);
    }

    return histo;
}

void ui_histo_free(struct ui_histo *histo)
{
    free(histo->series.list);
    free(histo->series.data);
}

static void ui_histo_init(struct ui_histo *histo, struct pos pos, struct dim dim)
{
    int16_t side = histo->s.pad.w + histo->s.axes.pad.w;
    int16_t head = histo->s.pad.h + histo->s.axes.pad.h + histo->s.value.font->glyph_h;
    histo->inner = make_pos(pos.x + side, pos.y + head);

    int16_t width = dim.w - side - histo->s.pad.w;
    int16_t height = dim.h - head - histo->s.pad.h;

    histo->row = make_dim(width, (histo->s.row.h * histo->series.cols)  + histo->s.row.pad);
    histo->series.rows = height / histo->row.h;

    histo->series.data = calloc(
            histo->series.rows * histo->series.len,
            sizeof(*histo->series.data));
}

void ui_histo_clear(struct ui_histo *histo)
{
    histo->v.bound = 1;
    histo->edge.t = 0;
    histo->edge.row = 0;
    histo->hover.active = false;

    memset(histo->series.data, 0,
            histo->series.rows * histo->series.len * sizeof(*histo->series.data));
}


ui_histo_data *ui_histo_at(struct ui_histo *histo, size_t row)
{
    if (!histo->series.data) return NULL;

    assert(histo->series.data);
    return histo->series.data + (row * histo->series.len);
}

ui_histo_data ui_histo_row_t(struct ui_histo *histo, size_t row)
{
    size_t delta = row > histo->edge.row ?
        histo->edge.row + (histo->series.rows - row) :
        histo->edge.row - row;

    return histo->edge.t -
        legion_min(histo->edge.t, delta * histo->t.scale);
}


void ui_histo_scale_t(struct ui_histo *histo, ui_histo_data scale)
{
    ui_histo_clear(histo);
    histo->t.scale = scale;
}

void ui_histo_advance(struct ui_histo *histo, ui_histo_data t)
{
    if (!histo->series.data) return;
    if (!histo->edge.t) { histo->edge.t = t; return; }

    while (t >= histo->edge.t + histo->t.scale) {
        histo->edge.t += histo->t.scale;
        histo->edge.row = (histo->edge.row + 1) % histo->series.rows;

        memset(histo->series.data + (histo->edge.row * histo->series.len),
                0, histo->series.len * sizeof(*histo->series.data));
    }

    histo->hover.t = ui_histo_row_t(histo, histo->hover.row);
}

void ui_histo_push(struct ui_histo *histo, size_t series, ui_histo_data v)
{
    assert(series < histo->series.len);
    if (!histo->series.data) return;

    ui_histo_at(histo, histo->edge.row)[series] += v;
}

static void ui_histo_update_v_bound(struct ui_histo *histo)
{
    ui_histo_data max = 0, sum = 0, col = 0;
    ui_histo_data *it = ui_histo_at(histo, histo->edge.row);

    for (size_t series = 0; series < histo->series.len; ++series) {
        if (col != histo->series.list[series].col) {
            max = legion_max(max, sum);
            sum = 0; col++;
        }

        sum += it[series];
    }

    max = legion_max(max, sum);
    while (max > histo->v.bound) histo->v.bound *= 2;
}


enum ui_ret ui_histo_event(struct ui_histo *histo, const SDL_Event *ev)
{
    switch (ev->type) {

    case SDL_MOUSEMOTION: {
        if (!histo->series.data) return ui_nil;

        SDL_Point cursor = render.cursor.point;
        struct SDL_Rect inner = {
            .x = histo->inner.x,
            .y = histo->inner.y,
            .w = histo->row.w,
            .h = histo->row.h * histo->series.rows,
        };

        if (!sdl_rect_contains(&inner, &cursor)) {
            histo->hover.active = false;
            return ui_nil;
        }

        histo->hover.active = true;
        histo->hover.row = (cursor.y - histo->inner.y) / histo->row.h;
        histo->hover.t = ui_histo_row_t(histo, histo->hover.row);
        histo->hover.v =
            ((cursor.x - histo->inner.x) * histo->v.bound) / histo->row.w;

        return ui_nil;
    }

    default: { return ui_nil; }
    }
}

void ui_histo_render(
        struct ui_histo *histo,
        struct ui_layout *layout,
        SDL_Renderer *renderer)
{
    ui_layout_add(layout, &histo->w);

    if (!histo->series.data)
        ui_histo_init(histo, histo->w.pos, histo->w.dim);
    ui_histo_update_v_bound(histo);

    const int16_t up = histo->w.pos.y + histo->s.pad.h;
    const int16_t down = histo->w.pos.y + histo->w.dim.h - histo->s.pad.h;
    const int16_t left = histo->w.pos.x + histo->s.pad.w;
    const int16_t right = histo->w.pos.x + histo->w.dim.w - histo->s.pad.w;

    // Axes
    {
        rgba_render(histo->s.axes.fg, renderer);

        int16_t y = up + histo->s.value.font->glyph_h;
        sdl_err(SDL_RenderDrawLine(renderer, left, up, left, down));
        sdl_err(SDL_RenderDrawLine(renderer, left, y, right, y));

        SDL_Point pos = {
            .x = histo->inner.x,
            .y = histo->w.pos.y + histo->s.pad.h
        };
        font_render_bg(
                histo->s.value.font, renderer, pos,
                histo->s.value.fg, histo->s.value.bg,
                "0u", 2);

        char bound_str[str_scaled_len] = {0};
        str_scaled(histo->v.bound, bound_str, sizeof(bound_str));

        pos.x = right - sizeof(bound_str) * histo->s.value.font->glyph_w;
        font_render_bg(
                histo->s.value.font, renderer, pos,
                histo->s.value.fg, histo->s.value.bg,
                bound_str, sizeof(bound_str));
    }

    // Hover - bg
    if (histo->hover.active) {
        SDL_Rect rect = {
            .x = histo->inner.x,
            .y = histo->inner.y + histo->hover.row * histo->row.h,
            .w = histo->row.w,
            .h = histo->row.h,
        };

        rgba_render(histo->s.hover.bg, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &rect));
    }

    // Data
    {
        struct pos pos = histo->inner;
        int16_t h = (histo->row.h - histo->s.row.pad) / histo->series.cols;
        ui_histo_data *it = histo->series.data;

        for (size_t row = 0; row < histo->series.rows; ++row) {
            pos.x = histo->inner.x;
            pos.y = histo->inner.y + (row * histo->row.h);

            size_t col = 0;
            for (size_t i = 0; i < histo->series.len; ++i, ++it) {
                struct ui_histo_series *series = histo->series.list + i;
                if (!*it) continue;

                if (series->col != col) {
                    pos.x = histo->inner.x;
                    pos.y += h;
                    col = series->col;
                }

                int16_t w = (*it * histo->row.w) / histo->v.bound;

                rgba_render(series->fg, renderer);
                sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                                    .x = pos.x, .y = pos.y, .w = w, .h = h }));

                rgba_render(histo->s.border, renderer);
                sdl_err(SDL_RenderDrawRect(renderer, &(SDL_Rect) {
                                    .x = pos.x, .y = pos.y, .w = w, .h = h }));

                pos.x += w;
            }
        }
    }

    // Edge
    {
        rgba_render(histo->s.edge, renderer);
        sdl_err(SDL_RenderFillRect(renderer, &(SDL_Rect) {
                            .x = left,
                            .y = histo->inner.y + (histo->edge.row * histo->row.h),
                            .w = histo->row.w,
                            .h = histo->row.h,
                        }));
    }

    // Hover - fg
    if (histo->hover.active) {
        int16_t x = render.cursor.point.x;
        rgba_render(histo->s.hover.fg, renderer);
        sdl_err(SDL_RenderDrawLine(renderer, x, up, x, down));

        char val_str[str_scaled_len] = {0};
        str_scaled(histo->hover.v, val_str, sizeof(val_str));

        SDL_Point pos = {
            .x = x - (sizeof(val_str) * histo->s.value.font->glyph_w) + 1,
            .y = up
        };

        font_render_bg(
                histo->s.value.font, renderer, pos,
                histo->s.value.fg, histo->s.value.bg,
                val_str, sizeof(val_str));
    }
}
