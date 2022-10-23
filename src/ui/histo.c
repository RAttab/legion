/* histo.c
   Rémi Attab (remi.attab@gmail.com), 10 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "histo.h"
#include "game/proxy.h"
#include "render/render.h"


// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_histo_style_default(struct ui_style *s)
{
    s->histo = (struct ui_histo_style) {
        .pad = make_dim(4, 4),
        .edge = make_rgba(0xFF, 0x00, 0x00, 0x33),
        .border = rgba_black(),

        .row = { .h = 8, .pad = 4 },

        .hover = {
            .fg = rgba_green(),
            .bg = ui_st.rgba.list.hover
        },

        .axes = {
            .pad = make_dim(1, 2),
            .fg = ui_st.rgba.fg
        },

        .value = {
            .font = ui_st.font.base,
            .fg = ui_st.rgba.fg,
            .bg = ui_st.rgba.bg,
        },
    };
}


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

        .legend = {
            .rows = 0,
            .list = calloc(len, sizeof(struct ui_histo_legend)),
            .time = ui_label_new(ui_str_c("time:")),
            .time_val = ui_label_new(ui_str_v(14)),
            .per_tick  = ui_label_new(ui_str_c("/t")),
            .scale = {
                .label = ui_label_new(ui_str_c("t-scale:")),
                .val = ui_input_new(8),
                .set = ui_button_new(ui_str_c(">")),
            },
        },
    };

    memcpy(histo.series.list, list, len * sizeof(*list));

    size_t col_start = 0;
    for (size_t i = 0; i < len; ++i) {
        assert(!i || list[i-1].col <= list[i].col);
        histo.series.cols = legion_max(histo.series.cols, list[i].col + 1LU);

        if (i && list[i-1].col != list[i].col) {
            histo.legend.rows = legion_max(histo.legend.rows, i - col_start);
            col_start = i;
        }

        struct ui_histo_legend *legend = histo.legend.list + i;
        *legend = (struct ui_histo_legend) {
            .name = ui_label_new_s(&ui_st.label.energy.saved, ui_str_c(list[i].name)),
            .val = ui_label_new(ui_str_v(str_scaled_len)),
            .tick = ui_label_new(ui_str_v(str_scaled_len)),
        };
        legend->name.s.fg = list[i].fg;
    }

    histo.legend.rows = legion_max(histo.legend.rows, len - col_start);
    histo.legend_h = histo.s.value.font->glyph_h * (3 + histo.legend.rows);

    ui_input_set(&histo.legend.scale.val, "1");
    return histo;
}

void ui_histo_free(struct ui_histo *histo)
{
    free(histo->series.list);
    free(histo->series.data);

    for (size_t i = 0; i < histo->series.len; ++i) {
        struct ui_histo_legend *legend = histo->legend.list + i;
        ui_label_free(&legend->name);
        ui_label_free(&legend->val);
        ui_label_free(&legend->tick);
    }
    free(histo->legend.list);

    ui_label_free(&histo->legend.time);
    ui_label_free(&histo->legend.time_val);
    ui_label_free(&histo->legend.per_tick);
    ui_label_free(&histo->legend.scale.label);
    ui_input_free(&histo->legend.scale.val);
    ui_button_free(&histo->legend.scale.set);

}

static void ui_histo_init(struct ui_histo *histo, struct pos pos, struct dim dim)
{

    int16_t side = histo->s.pad.w + histo->s.axes.pad.w;
    int16_t head =
        histo->legend_h +
        histo->s.pad.h +
        histo->s.axes.pad.h +
        histo->s.value.font->glyph_h;
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


struct ui_histo_series *ui_histo_series(struct ui_histo *histo, size_t i)
{
    assert(i < histo->series.len);
    return histo->series.list + i;
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

void ui_histo_advance(struct ui_histo *histo, ui_histo_data t)
{
    if (!histo->series.data) return;
    if (!histo->edge.t) { histo->edge.t = t; return; }
    ui_histo_update_v_bound(histo);

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

void ui_histo_update_legend(struct ui_histo *histo)
{
    if (!histo->hover.active) {
        ui_set_nil(&histo->legend.time_val);
        for (size_t i = 0; i < histo->series.len; ++i) {
            struct ui_histo_legend *legend = histo->legend.list + i;
            ui_set_nil(&legend->val);
            ui_set_nil(&legend->tick);
        }
        return;
    }

    ui_histo_data t = histo->hover.t;
    ui_histo_data scale = histo->hover.row == histo->edge.row ?
        proxy_time(render.proxy) - t : histo->t.scale;
    if (!scale) scale = 1;

    ui_str_setf(ui_set(&histo->legend.time_val), "%lu", t);

    ui_histo_data *data = ui_histo_at(histo, histo->hover.row);
    for (size_t i = 0; i < histo->series.len; ++i) {
        struct ui_histo_legend *legend = histo->legend.list + i;
        ui_str_set_scaled(ui_set(&legend->val), data[i]);
        ui_str_set_scaled(ui_set(&legend->tick), data[i] / scale);
    }
}

static void ui_histo_scale_set(struct ui_histo *histo)
{
    uint64_t scale = 0;
    if (ui_input_get_u64(&histo->legend.scale.val, &scale) && scale)
        ui_histo_scale_t(histo, scale);

    else render_log(st_error, "unable to set the scale to '%.*s'",
            (unsigned) histo->legend.scale.val.buf.len,
            histo->legend.scale.val.buf.c);
}

enum ui_ret ui_histo_event(struct ui_histo *histo, const SDL_Event *ev)
{
    enum ui_ret ret = ui_nil;
    if ((ret = ui_input_event(&histo->legend.scale.val, ev))) {
        if (ret != ui_action) return ret;
        ui_histo_scale_set(histo);
        return ret;
    }

    if ((ret = ui_button_event(&histo->legend.scale.set, ev))) {
        if (ret != ui_action) return ret;
        ui_histo_scale_set(histo);
        return ret;
    }

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

        ui_histo_update_legend(histo);
        break;
    }

    default: { break; }
    }

    return ui_nil;
}

static void ui_histo_render_legend(
        struct ui_histo *histo, SDL_Renderer *renderer)
{
    struct ui_layout layout = ui_layout_new(
            histo->w.pos, make_dim(histo->w.dim.w, histo->legend_h));

    size_t tab_col = layout.base.dim.w
        / histo->s.value.font->glyph_w
        / histo->series.cols;
    size_t tab_data = 12;

    {
        ui_layout_tab(&layout, (0 * tab_col));
        ui_label_render(&histo->legend.time, &layout, renderer);
        ui_layout_tab(&layout, (0 * tab_col) + tab_data);
        ui_label_render(&histo->legend.time_val, &layout, renderer);

        ui_layout_tab(&layout, (1 * tab_col));
        ui_label_render(&histo->legend.scale.label, &layout, renderer);
        ui_layout_tab(&layout, (1 * tab_col) + tab_data);
        ui_input_render(&histo->legend.scale.val, &layout, renderer);
        ui_button_render(&histo->legend.scale.set, &layout, renderer);

        ui_layout_next_row(&layout);
        ui_layout_sep_row(&layout);
    }

    size_t it[histo->series.cols];
    size_t end[histo->series.cols];

    for (size_t i = 0; i < histo->series.len; ++i) {
        struct ui_histo_series *series = histo->series.list + i;

        if (!i) { it[0] = 0; continue; }
        if (series->col == (series - 1)->col) continue;

        it[series->col] = i;
        end[series->col - 1] = i;

    }
    end[histo->series.cols - 1] = histo->series.len;

    for (size_t row = 0; row < histo->legend.rows; ++row) {
        bool empty = true;

        for (size_t col = 0; col < histo->series.cols; ++col) {

            while ( it[col] < end[col] &&
                    !histo->series.list[it[col]].visible)
                it[col]++;

            if (it[col] >= end[col]) continue;
            empty = false;

            struct ui_histo_legend *legend = histo->legend.list + it[col];

            ui_layout_tab(&layout, (col * tab_col));
            ui_label_render(&legend->name, &layout, renderer);

            ui_layout_tab(&layout, (col * tab_col) + tab_data);
            ui_label_render(&legend->val, &layout, renderer);

            ui_layout_sep_cols(&layout, 2);
            ui_label_render(&legend->tick, &layout, renderer);
            ui_label_render(&histo->legend.per_tick, &layout, renderer);

            it[col]++;
        }

        if (empty) ui_layout_sep_row(&layout);
        else ui_layout_next_row(&layout);
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

    ui_histo_render_legend(histo, renderer);
    ui_histo_update_v_bound(histo);

    const int16_t up = histo->w.pos.y + histo->legend_h + histo->s.pad.h;
    const int16_t down = histo->w.pos.y + histo->w.dim.h - histo->s.pad.h;
    const int16_t left = histo->w.pos.x + histo->s.pad.w;
    const int16_t right = histo->w.pos.x + histo->w.dim.w - histo->s.pad.w;

    // Axes
    {
        rgba_render(histo->s.axes.fg, renderer);

        int16_t y = up + histo->s.value.font->glyph_h;
        sdl_err(SDL_RenderDrawLine(renderer, left, up, left, down));
        sdl_err(SDL_RenderDrawLine(renderer, left, y, right, y));

        SDL_Point pos = { .x = histo->inner.x, .y = up };
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
                assert(w <= histo->row.w);

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
