/* layout.c
   Rémi Attab (remi.attab@gmail.com), 11 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "render/ui.h"


// -----------------------------------------------------------------------------
// layout
// -----------------------------------------------------------------------------

struct layout layout_new(struct pos pos, struct dim dim)
{
    return (struct layout) {
        .top = make_pos(pos.x, pos.y),
        .dim = make_dim(dim.w, dim.h),
        .pad = make_dim(0, 0),
        .pos = make_pos(dim.x, dim.y),
        .next_y = dim.y,
    };
}

void layout_add(struct layout *layout, struct widget *widget)
{
    if (wdiget->dim.h == layout_inf)
        widget->dim.h = layout->dim.h - (layout->pos.y - layout->top.y);
    if (wdiget->dim.w == layout_inf)
        widget->dim.w = layout->dim.w - (layout->pos.x - layout->top.x);

    assert(layout->pos.x + widget->dim.w < layout->top.x + layout->dim.w);
    assert(layout->pos.y + widget->dim.h < layout->top.y + layout->dim.h);

    widget->pos = layout->pos;
    layout->pos.x += widget->dim.w + layout->pad.w;
    layout->next_y = legion_max(layout->next_y, layout->pos.y + widget->dim.h);
}

void layout_next_row(struct layout *layout)
{
    layout->pos.x = layout->top.x;
    layout->pos.y = layout->next_y + layout->pad.h;
    layout->next_y = layout->pos.y;
}

void layout_sep_x(struct layout *layout, int16_t px)
{
    assert(layout->pos.x + px < layout->top.x + layout->dim.w);

    layout->pos.x += px;
}

void layout_sep_y(struct layout *layout, int16_t px)
{
    assert(layout->next_y == layout->pos.y);
    assert(layout->pos.y + px < layout->top.y + layout->dim.h);

    layout->pos.y += px;
    layout->next_y = layout->pos.y;
}