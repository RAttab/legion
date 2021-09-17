/* layout.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "ui/ui.h"


// -----------------------------------------------------------------------------
// layout
// -----------------------------------------------------------------------------

struct ui_layout ui_layout_new(struct pos pos, struct dim dim)
{
    return (struct ui_layout) {
        .top = make_pos(pos.x, pos.y),
        .dim = make_dim(dim.w, dim.h),
        .pad = make_dim(0, 0),
        .pos = make_pos(pos.x, pos.y),
        .next_y = pos.y,
    };
}

void ui_layout_add(struct ui_layout *layout, struct ui_widget *widget)
{
    if (widget->dim.h == ui_layout_inf)
        widget->dim.h = layout->dim.h - (layout->pos.y - layout->top.y);
    if (widget->dim.w == ui_layout_inf)
        widget->dim.w = layout->dim.w - (layout->pos.x - layout->top.x);

    assert(layout->pos.x + widget->dim.w <= layout->top.x + layout->dim.w);
    assert(layout->pos.y + widget->dim.h <= layout->top.y + layout->dim.h);

    widget->pos = layout->pos;
    layout->pos.x += widget->dim.w + layout->pad.w;
    layout->next_y = legion_max(layout->next_y, layout->pos.y + widget->dim.h);
}

struct ui_layout ui_layout_inner(struct ui_layout *layout)
{
    return ui_layout_new(layout->pos, make_dim(
                    layout->dim.w - (layout->pos.x - layout->top.x),
                    layout->next_y));
}

void ui_layout_next_row(struct ui_layout *layout)
{
    layout->pos.x = layout->top.x;
    layout->pos.y = layout->next_y + layout->pad.h;
    layout->next_y = layout->pos.y;
}

void ui_layout_sep_x(struct ui_layout *layout, int16_t px)
{
    assert(layout->pos.x + px < layout->top.x + layout->dim.w);

    layout->pos.x += px;
}

void ui_layout_sep_y(struct ui_layout *layout, int16_t px)
{
    assert(layout->next_y == layout->pos.y);
    assert(layout->pos.y + px < layout->top.y + layout->dim.h);

    layout->pos.y += px;
    layout->next_y = layout->pos.y;
}

void ui_layout_mid(struct ui_layout *layout, const struct ui_widget *widget)
{
    int16_t x = layout->top.x + (layout->dim.w/2 - widget->dim.w/2);
    assert(layout->pos.x < x);
    layout->pos.x = x;
}

void ui_layout_right(struct ui_layout *layout, const struct ui_widget *widget)
{
    int16_t x = layout->top.x + (layout->dim.w - layout->pad.w - widget->dim.w);
    assert(layout->pos.x < x);
    layout->pos.x = x;
}
