/* layout.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "layout.h"


// -----------------------------------------------------------------------------
// layout
// -----------------------------------------------------------------------------

void ui_layout_resize(struct ui_layout *layout, struct pos pos, struct dim dim)
{
    layout->base.pos = pos;
    layout->base.dim = dim;
    layout->row.pos = pos;
    layout->row.dim = (struct dim) { .w = dim.w, .h = 0 };
}

struct ui_layout ui_layout_new(struct pos pos, struct dim dim)
{
    struct ui_layout layout = { 0 };
    ui_layout_resize(&layout, pos, dim);
    layout.dir = ui_layout_right;
    return layout;
}

void ui_layout_add(struct ui_layout *layout, struct ui_widget *widget)
{
    if (widget->dim.w == ui_layout_inf)
        widget->dim.w = layout->row.dim.w;
    if (widget->dim.h == ui_layout_inf)
        widget->dim.h = layout->base.dim.h - (layout->row.pos.y - layout->base.pos.y);

    assert(layout->row.dim.w >= widget->dim.w);
    assert(layout->row.pos.y + widget->dim.h <= layout->base.pos.y + layout->base.dim.h);

    layout->row.dim.w -= widget->dim.w;
    layout->row.dim.h = legion_max(layout->row.dim.h, widget->dim.h);

    widget->pos = layout->row.pos;

    if (layout->dir == ui_layout_right)
        layout->row.pos.x += widget->dim.w;
    else widget->pos.x += layout->row.dim.w;
}

struct ui_layout ui_layout_inner(struct ui_layout *layout)
{
    return ui_layout_new(layout->row.pos, layout->row.dim);
}

struct ui_layout ui_layout_split_x(struct ui_layout *layout, int16_t width)
{
    assert(width <= layout->row.dim.w);

    int16_t height = layout->base.dim.h - (layout->row.pos.y - layout->base.pos.y);
    struct ui_layout inner = ui_layout_new(layout->row.pos, make_dim(width,  height));

    layout->base.pos.x = layout->row.pos.x + width;
    layout->row.pos.x = layout->base.pos.x;

    layout->base.dim.w = layout->row.dim.w - width;
    layout->row.dim.w = layout->base.dim.w;

    return inner;
}

void ui_layout_next_row(struct ui_layout *layout)
{
    layout->row.pos.x = layout->base.pos.x;
    layout->row.pos.y += layout->row.dim.h;

    layout->row.dim.w = layout->base.dim.w;
    layout->row.dim.h = 0;
}

void ui_layout_sep_x(struct ui_layout *layout, int16_t px)
{
    assert(layout->row.dim.w >= px);

    if (layout->dir == ui_layout_right)
        layout->row.pos.x += px;
    layout->row.dim.w -= px;
}

void ui_layout_sep_col(struct ui_layout *layout)
{
    ui_layout_sep_x(layout, ui_st.font.dim.w);
}

void ui_layout_sep_cols(struct ui_layout *layout, size_t n)
{
    ui_layout_sep_x(layout, n * ui_st.font.dim.w);
}

void ui_layout_sep_y(struct ui_layout *layout, int16_t px)
{
    assert(layout->row.dim.h == 0);
    assert(layout->row.pos.y + px <= layout->base.pos.y + layout->base.dim.h);

    layout->row.pos.y += px;
}

void ui_layout_sep_row(struct ui_layout *layout)
{
    ui_layout_sep_y(layout, ui_st.font.dim.h);
}

void ui_layout_mid(struct ui_layout *layout, int width)
{
    int x = layout->base.pos.x + (layout->base.dim.w/2 - width/2);
    assert(layout->row.pos.x < x);
    layout->row.pos.x = x;
    layout->row.dim.w = width;
}

void ui_layout_dir(struct ui_layout *layout, enum ui_layout_dir dir)
{
    assert(dir == ui_layout_right || dir == ui_layout_left);
    layout->dir = dir;
}
