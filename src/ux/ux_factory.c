/* factory.c
   Rémi Attab (remi.attab@gmail.com), 02 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


static void ux_factory_free(void *);
static void ux_factory_update(void *);
static void ux_factory_event(void *);
static void ux_factory_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// factory
// -----------------------------------------------------------------------------

typedef struct pos flow_pos;
typedef struct rect flow_rect;

constexpr size_t ux_factory_rows = im_rank_sys;
constexpr size_t ux_factory_cols = chunk_item_cap;
constexpr size_t ux_factory_grid_len = ux_factory_rows * ux_factory_cols;

struct ux_factory
{
    struct
    {
        struct dim margin, pad;
        struct rgba fg, bg, hover, select, border, op;
    } s;

    struct
    {
        coord_scale scale;
        flow_pos pos;
        flow_rect bbox;

        struct dim total, src, dst;
    } view;

    struct { struct ui_str id, item, num; } str;

    struct { bool panning, panned; } pan;

    struct
    {
        struct coord star;
        im_id select;

        struct workers workers;

        struct htable index;
        uint8_t rows, cols[ux_factory_rows];
        struct flow grid[ux_factory_grid_len];
    } state;
};

void ux_factory_alloc(struct ux_view_state *state)
{
    struct ux_factory *ux = mem_alloc_t(ux);
    *ux = (struct ux_factory) {

        .s = {
            .margin = make_dim(5, 5),
            .pad = make_dim(20, 20),

            .fg = ui_st.rgba.factory.fg,
            .bg = ui_st.rgba.factory.bg,
            .hover = ui_st.rgba.factory.hover,
            .select = ui_st.rgba.factory.select,
            .border = ui_st.rgba.factory.border,
            .op = ui_st.rgba.factory.op,
        },

        .view = { .scale = coord_scale_init() },

        .str = {
            .id = ui_str_v(im_id_str_len),
            .item = ui_str_v(item_str_len),
            .num = ui_str_v(3),
        },
    };

    struct dim inner = engine_dim(item_str_len + 1 + (2*3) + 1, 3);

    ux->view.total = make_dim(
            inner.w + 2*ux->s.margin.w + 2*ux->s.pad.w,
            inner.h + 2*ux->s.margin.h + 2*ux->s.pad.h);

    ux->view.src = make_dim(
            inner.w/2 + ux->s.margin.w + ux->s.pad.w,
            inner.h + 2*ux->s.margin.h);

    ux->view.dst = make_dim(
            inner.w/2 + ux->s.margin.w + ux->s.pad.w,
            0);

    *state = (struct ux_view_state) {
        .state = ux,
        .view = ux_view_factory,
        .slots = ux_slot_back,
        .fn = {
            .free = ux_factory_free,
            .update = ux_factory_update,
            .event = ux_factory_event,
            .render = ux_factory_render,
        },
    };
}

static void ux_factory_free(void *state)
{
    struct ux_factory *ux = state;

    ui_str_free(&ux->str.id);
    ui_str_free(&ux->str.item);
    ui_str_free(&ux->str.num);

    vec32_free(ux->state.workers.ops);
    htable_reset(&ux->state.index);

    mem_free(ux);
}

// -----------------------------------------------------------------------------
// coordinate system
// -----------------------------------------------------------------------------

static void ux_factory_update_bbox(struct ux_factory *ux)
{
    uint8_t cols = 0;
    for (size_t i = 0; i < ux->state.rows; ++i)
        cols = legion_max(cols, ux->state.cols[i]);

    struct dim bbox = {
        .w = 2 * (ux->state.rows * ux->view.total.w),
        .h = 2 * (cols * ux->view.total.w),
    };
    ux->view.bbox = make_rect(-bbox.w, -bbox.h, 2*bbox.w, 2*bbox.h);
    ux->view.pos = rect_clamp(ux->view.bbox, ux->view.pos);
}

static flow_pos ux_factory_to_flow_pos(struct ux_factory *ux, struct pos p)
{
    struct rect area = engine_area();
    assert(area.x == 0 && area.y == 0);
    return make_pos(
            coord_scale_mult(ux->view.scale, p.x - (area.w / 2)) + ux->view.pos.x,
            coord_scale_mult(ux->view.scale, p.y - (area.h / 2)) + ux->view.pos.y);
}

static flow_rect ux_factory_to_flow_rect(struct ux_factory *ux, struct rect r)
{
    return make_rect_parts(ux_factory_to_flow_pos(ux, rect_pos(r)), make_dim(
                    coord_scale_mult(ux->view.scale, r.w),
                    coord_scale_mult(ux->view.scale, r.h)));
}

static flow_rect ux_factory_flow_rect(
        struct ux_factory *ux, uint8_t row, uint8_t col)
{
    uint8_t cols = ux->state.cols[row];
    uint8_t rows = ux->state.rows;

    unit w = cols * ux->view.total.w;
    unit h = rows * ux->view.total.h;

    return make_rect(
        (col * ux->view.total.w) - w/2 + ux->s.pad.w,
        (row * ux->view.total.h) - h/2 + ux->s.pad.h,
        ux->view.total.w - 2*ux->s.pad.w,
        ux->view.total.h - 2*ux->s.pad.h);
}

static const struct flow *ux_factory_flow_at(struct ux_factory *ux, struct pos p)
{
    flow_pos pos = ux_factory_to_flow_pos(ux, p);

    const unit h = ux->state.rows * ux->view.total.h;
    const unit top = -h/2;
    if (pos.y < top || pos.y >= top + h) return nullptr;

    unit row = (pos.y - top) / ux->view.total.h;
    assert(row >= 0 && row < ux->state.rows);

    const unit w = ux->state.cols[row] * ux->view.total.w;
    const unit left = -w/2;
    if (pos.x < left || pos.x >= left + w) return nullptr;

    unit col = (pos.x - left) / ux->view.total.w;
    assert(col >= 0 && col < ux->state.cols[row]);

    if (!rect_contains(ux_factory_flow_rect(ux, row, col), pos))
        return nullptr;

    return ux->state.grid + (row * ux_factory_cols + col);
}


// -----------------------------------------------------------------------------
// interface
// -----------------------------------------------------------------------------

static void ux_factory_make_flow(
        struct ux_factory *ux, struct chunk *chunk, im_id id)
{
    const void *state = chunk_get(chunk, id);
    assert(state);

    const struct im_config *config = im_config_assert(im_id_item(id));
    assert(config->im.flow);

    struct flow flow = {0};
    if (!config->im.flow(state, &flow)) return;
    if (flow.rank >= ux_factory_rows) return;

    size_t col = ux->state.cols[flow.rank]++;
    size_t index = flow.rank * ux_factory_cols + col;

    ux->state.grid[index] = flow;
    ux->state.rows = legion_max(ux->state.rows, flow.rank + 1);

    struct htable_ret ret = htable_put(&ux->state.index, flow.id, index);
    assert(ret.ok);
}

static void ux_factory_update(void *state)
{
    struct ux_factory *ux = state;
    assert(!coord_is_nil(ux->state.star));

    ux->state.rows = 0;
    memset(ux->state.cols, 0, sizeof(ux->state.cols));
    htable_reset(&ux->state.index);

    if (ux->state.workers.ops) vec32_free(ux->state.workers.ops);
    memset(&ux->state.workers, 0, sizeof(ux->state.workers));

    struct chunk *chunk = proxy_chunk(ux->state.star);
    if (!chunk) return;

    struct vec16 *ids = chunk_list_filter(chunk, im_list_factory);
    for (size_t i = 0; i < vec16_len(ids); ++i)
        ux_factory_make_flow(ux, chunk, ids->vals[i]);
    vec16_free(ids);

    ux->state.workers = *chunk_workers(chunk);
    ux->state.workers.ops = vec32_copy(ux->state.workers.ops);

    ux_factory_update_bbox(ux);
}

static void ux_factory_focus(struct ux_factory *ux, im_id id)
{
    struct htable_ret ret = htable_get(&ux->state.index, id);
    if (!ret.ok) return;

    uint8_t row = ret.value / ux_factory_cols;
    uint8_t col = ret.value % ux_factory_cols;

    flow_rect r = ux_factory_flow_rect(ux, row, col);
    ux->view.pos = make_pos(r.x + (r.w / 2), r.y + r.h / 2);
}

void ux_factory_show(struct coord star, im_id id)
{
    struct ux_factory *ux = ux_state(ux_view_factory);
    assert(!coord_is_nil(star));

    ux->state.star = star;
    ux->state.select = 0;

    ux->view.pos = pos_nil();
    ux->view.scale = coord_scale_init();

    ux_factory_update(ux);

    if (id) {
        ux_factory_focus(ux, id);
        ux->state.select = id;
    }

    ux_show(ux_view_factory);
}

coord_scale ux_factory_scale(void)
{
    struct ux_factory *ux = ux_state(ux_view_factory);
    return ux->view.scale;
}

struct coord ux_factory_coord(void)
{
    struct ux_factory *ux = ux_state(ux_view_factory);
    return ux->state.star;
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static void ux_factory_event(void *state)
{
    struct ux_factory *ux = state;

    for (auto ev = ev_select_star(); ev; ev = nullptr)
        ux_factory_show(ev->star, 0);
    for (auto ev = ev_select_item(); ev; ev = nullptr)
        ux_factory_show(ev->star, ev->item);

    for (auto ev = ev_next_scroll(nullptr); ev; ev = ev_next_scroll(ev))
        ux->view.scale = coord_scale_inc(ux->view.scale, -ev->dy);

    for (auto ev = ev_mouse(); ev; ev = nullptr) {
        if (!ux->pan.panning) continue;
        if (!ev_button_down(ev_button_left)) {
            ux->pan.panning = ux->pan.panned = false;
            continue;
        }

        unit dx = coord_scale_mult(ux->view.scale, -ev->d.x);
        unit dy = coord_scale_mult(ux->view.scale, -ev->d.y);
        ux->view.pos = rect_clamp(ux->view.bbox, pos_add(ux->view.pos, dx, dy));

        ux->pan.panned = true;
    }

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;
        ev_consume_button(ev);

        if (ev->state == ev_state_down) ux->pan.panning = true;
        if (ev->state != ev_state_up) continue;

        ux->pan.panning = false;
        if (ux->pan.panned) { ux->pan.panned = false; continue; }

        const struct flow *flow = ux_factory_flow_at(ux, ev_mouse_pos());
        if (flow) ux_item_show(flow->id, ux->state.star);
    }
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void ux_factory_render_flow(
        struct ux_factory *ux,
        const flow_rect area,
        const render_layer layer,
        const uint8_t row, const uint8_t col)
{
    flow_rect rect = ux_factory_flow_rect(ux, row, col);
    if (!rect_intersects(area, rect)) return;

    const struct flow *flow = ux->state.grid + (row * ux_factory_cols + col);

    const render_layer layer_bg = layer + 0;
    const render_layer layer_fg = layer + 1;

    flow_pos cursor = ux_cursor_panel() ? make_pos(unit_min, unit_min) :
        ux_factory_to_flow_pos(ux, ev_mouse_pos());

    {
        struct rgba bg =
            rect_contains(rect, cursor) ? ux->s.hover :
            flow->id == ux->state.select ? ux->s.select :
            ux->s.bg;

        render_rect_fill_a(layer_bg, bg, rect, area);
        render_rect_line_a(layer_fg, ux->s.border, rect, area);
    }

    rect.x += ux->s.margin.w; rect.w -= 2 * ux->s.margin.w;
    rect.y += ux->s.margin.h; rect.h -= 2 * ux->s.margin.h;
    flow_pos pos = rect_pos(rect);
    struct dim cell = engine_cell();

    { // id
        ui_str_set_id(&ux->str.id, flow->id);
        render_font_a(
                layer_fg, font_base, ux->s.fg, pos, area,
                ux->str.id.str, ux->str.id.len);

    }

    pos.x = rect.x;
    pos.y += cell.h;

    do { // target - loop
        ui_str_set_item(&ux->str.item, flow->target);
        render_font_a(
                layer_fg, font_base,
                ux->s.fg, pos, area,
                ux->str.item.str, ux->str.item.len);


        if (flow->loops == im_loops_inf) break;

        ui_str_set_u64(&ux->str.num, flow->loops);
        pos.x = (rect.x + rect.w) - (ux->str.num.len * cell.w);
        render_font_a(
                layer_fg, font_base, ux->s.fg, pos, area,
                ux->str.num.str, ux->str.num.len);

        pos.x -= cell.w;
        render_font_a(layer_fg, font_base, ux->s.fg, pos, area, "x", 1);

    } while(false);

    pos.x = rect.x;
    pos.y += cell.h;

    do { // tape[i] - x of x
        struct rgba fg = {0};

        switch (flow->state)
        {

        case tape_input: {
            fg = ui_st.rgba.in;
            ui_str_set_item(&ux->str.item, flow->item);
            break;
        }

        case tape_output: {
            fg = ui_st.rgba.out;
            ui_str_set_item(&ux->str.item, flow->item);
            break;
        }

        case tape_work: {
            fg = ui_st.rgba.work;
            ui_str_setc(&ux->str.item, "work");
            break;
        }

        case tape_eof: {
            fg = ui_st.rgba.disabled;
            ui_str_setc(&ux->str.item, "end");
            break;
        }

        default: { assert(false); }
        }

        render_font_a(
                layer_fg, font_base, fg, pos, area,
                ux->str.item.str, ux->str.item.len);

        if (!flow->tape_len || flow->state == tape_eof) break;

        ui_str_set_u64(&ux->str.num, flow->tape_len);
        pos.x = (pos.x + rect.w) - (ux->str.num.len * cell.w);
        render_font_a(
                layer_fg, font_base, ux->s.fg, pos, area,
                ux->str.num.str, ux->str.num.len);

        // When a port is outputting, it only displays how many items are left
        // to output as it tries to reach zero and we don't have a tape_it to
        // work with.
        if (im_id_item(flow->id) == item_port && flow->state == tape_output)
            break;

        pos.x -= cell.w;
        render_font_a(layer_fg, font_base, ux->s.fg, pos, area, "/", 1);

        ui_str_set_u64(&ux->str.num, flow->tape_it + 1);
        pos.x -= ux->str.num.len * cell.w;
        render_font_a(
                layer_fg, font_base, ux->s.fg, pos, area,
                ux->str.num.str, ux->str.num.len);

    } while (false);
}

static void ux_factory_render_op(
        struct ux_factory *ux,
        const flow_rect area,
        const render_layer layer,
        const im_id src_id, const im_id dst_id)
{
    flow_pos src = {0};
    {
        struct htable_ret ret = htable_get(&ux->state.index, src_id);
        assert(ret.ok);

        flow_rect rect = ux_factory_flow_rect(ux,
                ret.value / ux_factory_cols,
                ret.value % ux_factory_cols);

        src = make_pos(rect.x + ux->view.src.w, rect.y + ux->view.src.h);
    }

    flow_pos dst = {0};
    {
        struct htable_ret ret = htable_get(&ux->state.index, dst_id);
        assert(ret.ok);

        flow_rect rect = ux_factory_flow_rect(ux,
                ret.value / ux_factory_cols,
                ret.value % ux_factory_cols);

        dst = make_pos(rect.x + ux->view.dst.w, rect.y + ux->view.dst.h);
    }

    render_line_a(layer, ux->s.op, (struct line) { src, dst }, area);
}

static void ux_factory_render(void *state, struct ui_layout *)
{
    struct ux_factory *ux = state;

    flow_rect area = ux_factory_to_flow_rect(ux, engine_area());
    const render_layer layer = render_layer_push(3);

    for (size_t i = 0; i < ux->state.rows; ++i)
        for (size_t j = 0; j < ux->state.cols[i]; ++j)
            ux_factory_render_flow(ux, area, layer + 0, i, j);

    for (size_t i = 0; i < vec32_len(ux->state.workers.ops); ++i) {
        im_id src = 0, dst = 0;
        chunk_workers_ops(ux->state.workers.ops->vals[i], &src, &dst);
        ux_factory_render_op(ux, area, layer + 2, src, dst);
    }

    render_layer_pop();
}
