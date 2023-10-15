/* factory.c
   Rémi Attab (remi.attab@gmail.com), 02 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "db/items.h"
#include "game/coord.h"
#include "game/chunk.h"
#include "items/types.h"
#include "items/config.h"
#include "utils/htable.h"
#include "utils/vec.h"

static void ui_factory_free(void *);
static void ui_factory_update(void *);
static void ui_factory_event(void *);
static void ui_factory_render(void *, struct ui_layout *);


// -----------------------------------------------------------------------------
// factory
// -----------------------------------------------------------------------------

typedef struct pos flow_pos;
typedef struct rect flow_rect;

constexpr size_t ui_factory_rows = im_rank_sys;
constexpr size_t ui_factory_cols = chunk_item_cap;
constexpr size_t ui_factory_grid_len = ui_factory_rows * ui_factory_cols;

struct ui_factory
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
        uint8_t rows, cols[ui_factory_rows];
        struct flow grid[ui_factory_grid_len];
    } state;
};

void ui_factory_alloc(struct ui_view_state *state)
{
    struct ui_factory *ui = calloc(1, sizeof (*ui));
    *ui = (struct ui_factory) {

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

    ui->view.total = make_dim(
            inner.w + 2*ui->s.margin.w + 2*ui->s.pad.w,
            inner.h + 2*ui->s.margin.h + 2*ui->s.pad.h);

    ui->view.src = make_dim(
            inner.w/2 + ui->s.margin.w + ui->s.pad.w,
            inner.h + 2*ui->s.margin.h);

    ui->view.dst = make_dim(
            inner.w/2 + ui->s.margin.w + ui->s.pad.w,
            0);

    *state = (struct ui_view_state) {
        .state = ui,
        .view = ui_view_factory,
        .slots = ui_slot_back,
        .fn = {
            .free = ui_factory_free,
            .update_frame = ui_factory_update,
            .event = ui_factory_event,
            .render = ui_factory_render,
        },
    };
}

static void ui_factory_free(void *state)
{
    struct ui_factory *ui = state;

    ui_str_free(&ui->str.id);
    ui_str_free(&ui->str.item);
    ui_str_free(&ui->str.num);

    vec32_free(ui->state.workers.ops);
    htable_reset(&ui->state.index);

    free(ui);
}

// -----------------------------------------------------------------------------
// coordinate system
// -----------------------------------------------------------------------------

static void ui_factory_update_bbox(struct ui_factory *ui)
{
    uint8_t cols = 0;
    for (size_t i = 0; i < ui->state.rows; ++i)
        cols = legion_max(cols, ui->state.cols[i]);

    struct dim bbox = {
        .w = 2 * (ui->state.rows * ui->view.total.w),
        .h = 2 * (cols * ui->view.total.w),
    };
    ui->view.bbox = make_rect(-bbox.w, -bbox.h, 2*bbox.w, 2*bbox.h);
    ui->view.pos = rect_clamp(ui->view.bbox, ui->view.pos);
}

static flow_pos ui_factory_to_flow_pos(struct ui_factory *ui, struct pos p)
{
    struct rect area = engine_area();
    assert(area.x == 0 && area.y == 0);
    return make_pos(
            coord_scale_mult(ui->view.scale, p.x - (area.w / 2)) + ui->view.pos.x,
            coord_scale_mult(ui->view.scale, p.y - (area.h / 2)) + ui->view.pos.y);
}

static flow_rect ui_factory_to_flow_rect(struct ui_factory *ui, struct rect r)
{
    return make_rect_parts(ui_factory_to_flow_pos(ui, rect_pos(r)), make_dim(
                    coord_scale_mult(ui->view.scale, r.w),
                    coord_scale_mult(ui->view.scale, r.h)));
}

static flow_rect ui_factory_flow_rect(
        struct ui_factory *ui, uint8_t row, uint8_t col)
{
    uint8_t cols = ui->state.cols[row];
    uint8_t rows = ui->state.rows;

    unit w = cols * ui->view.total.w;
    unit h = rows * ui->view.total.h;

    return make_rect(
        (col * ui->view.total.w) - w/2 + ui->s.pad.w,
        (row * ui->view.total.h) - h/2 + ui->s.pad.h,
        ui->view.total.w - 2*ui->s.pad.w,
        ui->view.total.h - 2*ui->s.pad.h);
}

static const struct flow *ui_factory_flow_at(struct ui_factory *ui, struct pos p)
{
    flow_pos pos = ui_factory_to_flow_pos(ui, p);

    const unit h = ui->state.rows * ui->view.total.h;
    const unit top = -h/2;
    if (pos.y < top || pos.y >= top + h) return nullptr;

    unit row = (pos.y - top) / ui->view.total.h;
    assert(row >= 0 && row < ui->state.rows);

    const unit w = ui->state.cols[row] * ui->view.total.w;
    const unit left = -w/2;
    if (pos.x < left || pos.x >= left + w) return nullptr;

    unit col = (pos.x - left) / ui->view.total.w;
    assert(col >= 0 && col < ui->state.cols[row]);

    if (!rect_contains(ui_factory_flow_rect(ui, row, col), pos))
        return nullptr;

    return ui->state.grid + (row * ui_factory_cols + col);
}


// -----------------------------------------------------------------------------
// interface
// -----------------------------------------------------------------------------

static void ui_factory_make_flow(
        struct ui_factory *ui, struct chunk *chunk, im_id id)
{
    const void *state = chunk_get(chunk, id);
    assert(state);

    const struct im_config *config = im_config_assert(im_id_item(id));
    assert(config->im.flow);

    struct flow flow = {0};
    if (!config->im.flow(state, &flow)) return;
    if (flow.rank >= ui_factory_rows) return;

    size_t col = ui->state.cols[flow.rank]++;
    size_t index = flow.rank * ui_factory_cols + col;

    ui->state.grid[index] = flow;
    ui->state.rows = legion_max(ui->state.rows, flow.rank + 1);

    struct htable_ret ret = htable_put(&ui->state.index, flow.id, index);
    assert(ret.ok);
}

static void ui_factory_update(void *state)
{
    struct ui_factory *ui = state;
    assert(!coord_is_nil(ui->state.star));

    ui->state.rows = 0;
    memset(ui->state.cols, 0, sizeof(ui->state.cols));
    htable_reset(&ui->state.index);

    if (ui->state.workers.ops) vec32_free(ui->state.workers.ops);
    memset(&ui->state.workers, 0, sizeof(ui->state.workers));

    struct chunk *chunk = proxy_chunk(ui->state.star);
    if (!chunk) return;

    struct vec16 *ids = chunk_list_filter(chunk, im_list_factory);
    for (size_t i = 0; i < vec16_len(ids); ++i)
        ui_factory_make_flow(ui, chunk, ids->vals[i]);
    vec16_free(ids);

    ui->state.workers = chunk_workers(chunk);
    ui->state.workers.ops = vec32_copy(ui->state.workers.ops);

    ui_factory_update_bbox(ui);
}

static void ui_factory_focus(struct ui_factory *ui, im_id id)
{
    struct htable_ret ret = htable_get(&ui->state.index, id);
    if (!ret.ok) return;

    uint8_t row = ret.value / ui_factory_cols;
    uint8_t col = ret.value % ui_factory_cols;

    flow_rect r = ui_factory_flow_rect(ui, row, col);
    ui->view.pos = make_pos(r.x + (r.w / 2), r.y + r.h / 2);
}

void ui_factory_show(struct coord star, im_id id)
{
    struct ui_factory *ui = ui_state(ui_view_factory);
    assert(!coord_is_nil(star));

    ui->state.star = star;
    ui->state.select = 0;

    ui->view.pos = pos_nil();
    ui->view.scale = coord_scale_init();

    ui_factory_update(ui);

    if (id) {
        ui_factory_focus(ui, id);
        ui->state.select = id;
    }

    ui_show(ui_view_factory);
}

coord_scale ui_factory_scale(void)
{
    struct ui_factory *ui = ui_state(ui_view_factory);
    return ui->view.scale;
}

struct coord ui_factory_coord(void)
{
    struct ui_factory *ui = ui_state(ui_view_factory);
    return ui->state.star;
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static void ui_factory_event(void *state)
{
    struct ui_factory *ui = state;

    for (auto ev = ev_select_star(); ev; ev = nullptr)
        ui_factory_show(ev->star, 0);
    for (auto ev = ev_select_item(); ev; ev = nullptr)
        ui_factory_show(ev->star, ev->item);

    for (auto ev = ev_next_scroll(nullptr); ev; ev = ev_next_scroll(ev))
        ui->view.scale = coord_scale_inc(ui->view.scale, -ev->dy);

    for (auto ev = ev_mouse(); ev; ev = nullptr) {
        if (!ui->pan.panning) continue;
        if (!ev_button_down(ev_button_left)) {
            ui->pan.panning = ui->pan.panned = false;
            continue;
        }

        unit dx = coord_scale_mult(ui->view.scale, -ev->d.x);
        unit dy = coord_scale_mult(ui->view.scale, -ev->d.y);
        ui->view.pos = rect_clamp(ui->view.bbox, pos_add(ui->view.pos, dx, dy));

        ui->pan.panned = true;
    }

    for (auto ev = ev_next_button(nullptr); ev; ev = ev_next_button(ev)) {
        if (ev->button != ev_button_left) continue;
        ev_consume_button(ev);

        if (ev->state == ev_state_down) ui->pan.panning = true;
        if (ev->state != ev_state_up) continue;

        ui->pan.panning = false;
        if (ui->pan.panned) { ui->pan.panned = false; continue; }

        const struct flow *flow = ui_factory_flow_at(ui, ev_mouse_pos());
        if (flow) ui_item_show(flow->id, ui->state.star);
    }
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void ui_factory_render_flow(
        struct ui_factory *ui,
        const flow_rect area,
        const render_layer layer,
        const uint8_t row, const uint8_t col)
{
    flow_rect rect = ui_factory_flow_rect(ui, row, col);
    if (!rect_intersects(area, rect)) return;

    const struct flow *flow = ui->state.grid + (row * ui_factory_cols + col);

    const render_layer layer_bg = layer + 0;
    const render_layer layer_fg = layer + 1;

    {
        struct rgba bg =
            rect_contains(rect, ui_factory_to_flow_pos(ui, ev_mouse_pos())) ? ui->s.hover :
            flow->id == ui->state.select ? ui->s.select :
            ui->s.bg;

        render_rect_fill_a(layer_bg, bg, rect, area);
        render_rect_line_a(layer_fg, ui->s.border, rect, area);
    }

    rect.x += ui->s.margin.w; rect.w -= 2 * ui->s.margin.w;
    rect.y += ui->s.margin.h; rect.h -= 2 * ui->s.margin.h;
    flow_pos pos = rect_pos(rect);
    struct dim cell = engine_cell();

    { // id
        ui_str_set_id(&ui->str.id, flow->id);
        render_font_a(
                layer_fg, font_base, ui->s.fg, pos, area,
                ui->str.id.str, ui->str.id.len);

    }

    pos.x = rect.x;
    pos.y += cell.h;

    do { // target - loop
        ui_str_set_item(&ui->str.item, flow->target);
        render_font_a(
                layer_fg, font_base,
                ui->s.fg, pos, area,
                ui->str.item.str, ui->str.item.len);


        if (flow->loops == im_loops_inf) break;

        ui_str_set_u64(&ui->str.num, flow->loops);
        pos.x = (rect.x + rect.w) - (ui->str.num.len * cell.w);
        render_font_a(
                layer_fg, font_base, ui->s.fg, pos, area,
                ui->str.num.str, ui->str.num.len);

        pos.x -= cell.w;
        render_font_a(layer_fg, font_base, ui->s.fg, pos, area, "x", 1);

    } while(false);

    pos.x = rect.x;
    pos.y += cell.h;

    do { // tape[i] - x of x
        struct rgba fg = {0};

        switch (flow->state)
        {

        case tape_input: {
            fg = ui_st.rgba.in;
            ui_str_set_item(&ui->str.item, flow->item);
            break;
        }

        case tape_output: {
            fg = ui_st.rgba.out;
            ui_str_set_item(&ui->str.item, flow->item);
            break;
        }

        case tape_work: {
            fg = ui_st.rgba.work;
            ui_str_setc(&ui->str.item, "work");
            break;
        }

        case tape_eof: {
            fg = ui_st.rgba.disabled;
            ui_str_setc(&ui->str.item, "end");
            break;
        }

        default: { assert(false); }
        }

        render_font_a(
                layer_fg, font_base, fg, pos, area,
                ui->str.item.str, ui->str.item.len);

        if (!flow->tape_len || flow->state == tape_eof) break;

        ui_str_set_u64(&ui->str.num, flow->tape_len);
        pos.x = (pos.x + rect.w) - (ui->str.num.len * cell.w);
        render_font_a(
                layer_fg, font_base, ui->s.fg, pos, area,
                ui->str.num.str, ui->str.num.len);

        // When a port is outputting, it only displays how many items are left
        // to output as it tries to reach zero and we don't have a tape_it to
        // work with.
        if (im_id_item(flow->id) == item_port && flow->state == tape_output)
            break;

        pos.x -= cell.w;
        render_font_a(layer_fg, font_base, ui->s.fg, pos, area, "/", 1);

        ui_str_set_u64(&ui->str.num, flow->tape_it + 1);
        pos.x -= ui->str.num.len * cell.w;
        render_font_a(
                layer_fg, font_base, ui->s.fg, pos, area,
                ui->str.num.str, ui->str.num.len);

    } while (false);
}

static void ui_factory_render_op(
        struct ui_factory *ui,
        const flow_rect area,
        const render_layer layer,
        const im_id src_id, const im_id dst_id)
{
    flow_pos src = {0};
    {
        struct htable_ret ret = htable_get(&ui->state.index, src_id);
        assert(ret.ok);

        flow_rect rect = ui_factory_flow_rect(ui,
                ret.value / ui_factory_cols,
                ret.value % ui_factory_cols);

        src = make_pos(rect.x + ui->view.src.w, rect.y + ui->view.src.h);
    }

    flow_pos dst = {0};
    {
        struct htable_ret ret = htable_get(&ui->state.index, dst_id);
        assert(ret.ok);

        flow_rect rect = ui_factory_flow_rect(ui,
                ret.value / ui_factory_cols,
                ret.value % ui_factory_cols);

        dst = make_pos(rect.x + ui->view.dst.w, rect.y + ui->view.dst.h);
    }

    render_line_a(layer, ui->s.op, (struct line) { src, dst }, area);
}

static void ui_factory_render(void *state, struct ui_layout *)
{
    struct ui_factory *ui = state;

    flow_rect area = ui_factory_to_flow_rect(ui, engine_area());
    const render_layer layer = render_layer_push(3);

    for (size_t i = 0; i < ui->state.rows; ++i)
        for (size_t j = 0; j < ui->state.cols[i]; ++j)
            ui_factory_render_flow(ui, area, layer + 0, i, j);

    for (size_t i = 0; i < vec32_len(ui->state.workers.ops); ++i) {
        im_id src = 0, dst = 0;
        chunk_workers_ops(ui->state.workers.ops->vals[i], &src, &dst);
        ui_factory_render_op(ui, area, layer + 2, src, dst);
    }

    render_layer_pop();
}
