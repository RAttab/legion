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

        .v = { .bound = 0 },
        .t = { .scale = 1, .start = 0 },

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
        histo.series.cols = legion_max(histo.series.cols, list[i].col);
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
    histo->inner = make_pos(pos.x + head, pos.y + side);

    int16_t width = dim.w - side - histo->s.pad.w;
    int16_t height = dim.h - head - histo->s.pad.h;

    histo->series.rows = height / (histo->s.row.h + histo->s.row.pad);
    histo->row = make_dim(width, height / histo->series.rows);

    histo->series.data = calloc(
            histo->series.rows * histo->series.len,
            sizeof(*histo->series.data));
}

void ui_histo_clear(struct ui_histo *histo)
{
    histo->t.start = 0;
    histo->v.bound = 0;

    histo->edge.t = 0;
    histo->edge.row = 0;

    histo->hover.active = false;

    memset(histo->series.data, 0,
            histo->series.rows * histo->series.len * sizeof(*histo->series.data));
}


ui_histo_data *ui_histo_at(struct ui_histo *histo, size_t row)
{
    assert(histo->series.data);
    return histo->series.data + (row * histo->series.len);
}

ui_histo_data *ui_histo_at_t(struct ui_histo *histo, ui_histo_data t)
{
    assert(t >= histo->t.start);
    return ui_histo_at(histo,
            ((t - histo->t.start) / histo->t.scale) % histo->series.rows);
}

ui_histo_data *ui_histo_at_series(
        struct ui_histo *histo, size_t series, size_t row)
{
    assert(series < histo->series.len);
    return ui_histo_at(histo, row) + series;
}

ui_histo_data *ui_histo_at_series_t(
        struct ui_histo *histo, size_t series, ui_histo_data t)
{
    assert(series < histo->series.len);
    return ui_histo_at_t(histo, t) + series;
}


void ui_histo_scale_t(struct ui_histo *histo, ui_histo_data scale)
{
    ui_histo_clear(histo);
    histo->t.scale = scale;
}

void ui_histo_advance(struct ui_histo *histo, ui_histo_data t)
{
    if (!histo->series.data) return;
    if (!histo->t.start) { histo->t.start = t; return; }

    while (t > histo->edge.t + histo->t.scale) {
        histo->edge.t += histo->t.scale;
        histo->edge.row = (histo->edge.row + 1) % histo->series.rows;

        memset(ui_histo_at_t(histo, histo->edge.t), 0,
                histo->series.len * sizeof(*histo->series.data));
    }
}

void ui_histo_push(struct ui_histo *histo, size_t series, ui_histo_data v)
{
    *ui_histo_at_series_t(histo, series, histo->edge.t) += v;

    while (v > histo->v.bound) histo->v.bound *= 10;
}


enum ui_ret ui_histo_event(struct ui_histo *histo, const SDL_Event *ev)
{
    switch (ev->type) {

    case SDL_MOUSEMOTION: {
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
        histo->hover.v =
            ((cursor.x - histo->inner.x) * histo->v.bound) / histo->row.w;

        size_t delta = histo->hover.row > histo->edge.row ?
            histo->edge.row + (histo->series.rows - histo->hover.row) :
            histo->edge.row - histo->hover.row;
        histo->hover.t = histo->edge.t - (delta * histo->t.scale);

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
        ui_histo_init(histo, layout->base.pos, layout->base.dim);

    const int16_t up = layout->base.pos.y + histo->s.pad.h;
    const int16_t down = layout->base.pos.y + layout->base.dim.h - histo->s.pad.h;
    const int16_t left = layout->base.pos.x + histo->s.pad.w;
    const int16_t right = layout->base.pos.x + layout->base.dim.w - histo->s.pad.w;

    // Axes
    {
        rgba_render(histo->s.axes.fg, renderer);

        int16_t x = layout->base.pos.x + histo->s.pad.w;
        sdl_err(SDL_RenderDrawLine(renderer, x, up, x, down));

        int16_t y = layout->base.pos.y + histo->s.pad.h + histo->s.value.font->glyph_h;
        sdl_err(SDL_RenderDrawLine(renderer, left, y, right, y));

        SDL_Point pos = {.x = x + 1, .y = layout->base.pos.y + histo->s.pad.h };
        font_render_bg(
                histo->s.value.font, renderer, pos,
                histo->s.value.fg, histo->s.value.bg,
                "0", 1);

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
            pos.y = histo->inner.y + row * histo->row.h;

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

                pos.x += w;
            }
        }
    }

    // Edge
    {
        int16_t y = histo->inner.y + ((histo->edge.row + 1) * histo->row.h);
        rgba_render(histo->s.edge, renderer);
        sdl_err(SDL_RenderDrawLine(renderer,left, y, right, y));
    }

    // Hover - fg
    if (histo->hover.active) {
        int16_t x = render.cursor.point.x;
        rgba_render(histo->s.hover.fg, renderer);
        sdl_err(SDL_RenderDrawLine(renderer, x, up, x, down));

        char val_str[str_scaled_len] = {0};
        str_scaled(histo->hover.v, val_str, sizeof(val_str));

        SDL_Point pos = { .x = x, .y = up };
        if (x < left + ((left - right) / 2)) pos.x++;
        else pos.x -= (sizeof(val_str) * histo->s.value.font->glyph_w) + 1;

        font_render_bg(
                histo->s.value.font, renderer, pos,
                histo->s.value.fg, histo->s.value.bg,
                val_str, sizeof(val_str));
    }
}
