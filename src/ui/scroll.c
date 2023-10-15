/* scroll.c
   Rémi Attab (remi.attab@gmail.com), 12 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

// -----------------------------------------------------------------------------
// style
// -----------------------------------------------------------------------------

void ui_scroll_style_default(struct ui_style *s)
{
    s->scroll = (struct ui_scroll_style) {
        .fg = rgba_gray(0x88),
        .bg = s->rgba.bg,
        .width = engine_cell().w,
    };
}


// -----------------------------------------------------------------------------
// scroll
// -----------------------------------------------------------------------------

struct ui_scroll ui_scroll_new(struct dim dim, struct dim cell)
{
    return (struct ui_scroll) {
        .w = make_ui_widget(dim),
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

static struct rect ui_scroll_rect_rows(struct ui_scroll *scroll)
{
    if (!scroll->rows.show) return rect_nil();

    unit height = scroll->w.h;
    if (scroll->cols.total) height -= scroll->cell.h;

    struct rect rect = {
        .x = scroll->w.x + (scroll->w.w - scroll->cell.w),
        .y = scroll->w.y + ((height * scroll->rows.first) / scroll->rows.total),
        .w = scroll->s.width,
        .h = (height * scroll->rows.visible) / scroll->rows.total,
    };

    rect.x += (scroll->cell.w / 2) - (rect.w / 2);

    const unit max = scroll->w.y + height;
    if (rect.y + rect.h > max) rect.h = max - rect.y;

    return rect;
}

static struct rect ui_scroll_rect_cols(struct ui_scroll *scroll)
{
    if (!scroll->cols.show) return rect_nil();

    unit width = scroll->w.w;
    if (scroll->rows.total) width -= scroll->cell.w;

    struct rect rect = {
        .x = scroll->w.x + ((width * scroll->cols.first) / scroll->cols.total),
        .y = scroll->w.y + (scroll->w.h - scroll->cell.h),
        .w = (width * scroll->cols.visible) / scroll->cols.total,
        .h = scroll->s.width,
    };

    rect.y += (scroll->cell.h / 2) - (rect.h / 2);

    const unit max = scroll->w.x + width;
    if (rect.x + rect.w > max) rect.w = max - rect.x;

    return rect;
}


void ui_scroll_event(struct ui_scroll *scroll)
{
    for (auto ev = ev_next_scroll(nullptr); ev; ev = ev_next_scroll(ev)) {
        if (!ev_mouse_in(scroll->w)) continue;

        ui_scroll_move_rows(scroll, -ev->dy);
        ui_scroll_move_cols(scroll, ev->dx);
        ev_consume_scroll(ev);
    }

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;

        struct pos cursor = ev_mouse_pos();
        switch (ev->state)
        {

        case ev_state_down: {
            struct rect rows = ui_scroll_rect_rows(scroll);
            if (rect_contains(rows, cursor)) {
                scroll->drag.type = ui_scroll_rows;
                scroll->drag.start = cursor.y;
                scroll->drag.bar = rows.y;
                ev_consume_button(ev);
                break;
            }

            struct rect cols = ui_scroll_rect_cols(scroll);
            if (rect_contains(cols, cursor)) {
                scroll->drag.type = ui_scroll_cols;
                scroll->drag.start = cursor.x;
                scroll->drag.bar = cols.x;
                ev_consume_button(ev);
                break;
            }
        }

        case ev_state_up: {
            if (!scroll->drag.type) break;
            memset(&scroll->drag, 0, sizeof(scroll->drag));
            ev_consume_button(ev);
            break;
        }

        default: { break; }
        }
    }

    for (auto ev = ev_mouse(); ev; ev = nullptr) {
        switch (scroll->drag.type)
        {
        case ui_scroll_nil: { break; }

        case ui_scroll_rows: {
            unit delta = ev_mouse_pos().y - scroll->drag.start;
            unit bar = scroll->drag.bar + delta;
            if (bar < scroll->w.y) scroll->rows.first = 0;
            else {
                size_t first = ((bar - scroll->w.y) * scroll->rows.total) / scroll->w.h;
                scroll->rows.first = legion_min(scroll->rows.total, first);
            }
            break;
        }

        case ui_scroll_cols: {
            unit delta = ev_mouse_pos().x - scroll->drag.start;
            unit bar = scroll->drag.bar + delta;
            if (bar < scroll->w.x) scroll->cols.first = 0;
            else {
                size_t first = ((bar - scroll->w.x) * scroll->cols.total) / scroll->w.h;
                scroll->cols.first = legion_min(scroll->cols.total, first);
            }
            break;
        }

        default: { assert(false); }
        }
    }

}

struct ui_layout ui_scroll_render(struct ui_scroll *scroll, struct ui_layout *layout)
{
    ui_layout_add(layout, &scroll->w);

    const render_layer layer_bg = render_layer_push(2);
    const render_layer layer_fg = layer_bg + 1;

    scroll->rows.visible = scroll->w.h / scroll->cell.h;
    scroll->cols.visible = scroll->w.w / scroll->cell.w;

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

    if (!rgba_is_nil(scroll->s.bg))
        render_rect_fill(layer_bg, scroll->s.bg, scroll->w);

    if (scroll->rows.show)
        render_rect_fill(layer_fg, scroll->s.fg, ui_scroll_rect_rows(scroll));

    if (scroll->cols.show)
        render_rect_fill(layer_fg, scroll->s.fg, ui_scroll_rect_cols(scroll));

    // The inner layout doesn't overlap with the scroll bars so we can safely
    // pop the layers.
    render_layer_pop();

    struct dim dim = make_dim(
            scroll->w.w - (scroll->rows.show ? scroll->cell.w : 0),
            scroll->w.h - (scroll->cols.show ? scroll->cell.h : 0));
    return ui_layout_new(rect_pos(scroll->w), dim);
}
