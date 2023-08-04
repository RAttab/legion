/* factory.c
   Rémi Attab (remi.attab@gmail.com), 02 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/chunk.h"
#include "game/coord.h"
#include "db/io.h"
#include "db/items.h"
#include "items/types.h"
#include "render/ui.h"
#include "utils/vec.h"
#include "utils/htable.h"

static void ui_factory_free(void *);
static void ui_factory_update(void *, struct proxy *);
static bool ui_factory_event(void *, SDL_Event *);
static void ui_factory_render(void *, struct ui_layout *, SDL_Renderer *);


// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

#define vecx_type struct flow
#define vecx_name vec_flow
#include "utils/vecx.h"


// -----------------------------------------------------------------------------
// factory
// -----------------------------------------------------------------------------

struct flow_pos { int32_t x, y; };
struct flow_rect { int32_t x, y, w, h; };

struct ui_factory
{
    struct coord star;

    struct vec64 *grid;
    struct vec_flow *flows;
    struct htable index;
    struct workers workers;

    coord_scale scale;
    struct flow_pos pos;
    struct dim inner, margin, pad, total, in, out;

    SDL_Texture *tex;
    SDL_Rect tex_rect;

    struct ui_label ui_id;
    struct ui_label ui_target, ui_loops, ui_loops_val;
    struct ui_label ui_tape_in, ui_tape_work, ui_tape_out;
    struct ui_label ui_tape_num, ui_tape_of;

    bool panning, panned;
};

void ui_factory_alloc(struct ui_view_state *state)
{
    struct ui_factory *ui = calloc(1, sizeof (*ui));
    *ui = (struct ui_factory) {
        .scale = scale_init(),
        .pos = (struct flow_pos) {0},

        .ui_id = ui_label_new(ui_str_v(im_id_str_len)),
        .ui_target = ui_label_new(ui_str_v(item_str_len)),
        .ui_loops = ui_label_new(ui_str_c("x")),
        .ui_loops_val = ui_label_new(ui_str_v(3)),
        .ui_tape_in = ui_label_new_s(&ui_st.label.in, ui_str_v(item_str_len)),
        .ui_tape_work = ui_label_new_s(&ui_st.label.work, ui_str_c("work            ")),
        .ui_tape_out = ui_label_new_s(&ui_st.label.out, ui_str_v(item_str_len)),
        .ui_tape_num = ui_label_new(ui_str_v(3)),
        .ui_tape_of = ui_label_new(ui_str_c("/")),

        .inner = make_dim(
                (item_str_len + 3 + 1 + 3) * ui_st.font.base->glyph_w,
                3 * ui_st.font.base->glyph_h),
        .margin = make_dim(5, 5),
        .pad = make_dim(20, 20),
    };

    ui->total = make_dim(
            ui->inner.w + 2*ui->margin.w + 2*ui->pad.w,
            ui->inner.h + 2*ui->margin.h + 2*ui->pad.h);

    ui->in = make_dim(
            ui->pad.w + ui->margin.w + ui->inner.w / 2,
            ui->pad.h);

    ui->out = make_dim(
            ui->pad.w + ui->margin.w + ui->inner.w / 2,
            ui->pad.h + 2*ui->margin.h + ui->inner.h);


    ui->tex_rect = (SDL_Rect) {
        .x = 0, .y = 0,
        .w = ui->total.w,
        .h = ui->total.h,
    };

    ui->tex = sdl_ptr(SDL_CreateTexture(
                    render.renderer,
                    SDL_PIXELFORMAT_RGBA8888,
                    SDL_TEXTUREACCESS_TARGET,
                    ui->tex_rect.w, ui->tex_rect.h));

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

    ui_label_free(&ui->ui_id);
    ui_label_free(&ui->ui_target);
    ui_label_free(&ui->ui_loops);
    ui_label_free(&ui->ui_loops_val);
    ui_label_free(&ui->ui_tape_in);
    ui_label_free(&ui->ui_tape_work);
    ui_label_free(&ui->ui_tape_out);
    ui_label_free(&ui->ui_tape_num);
    ui_label_free(&ui->ui_tape_of);

    for (size_t i = 0; i < vec64_len(ui->grid); ++i)
        vec16_free((struct vec16 *) ui->grid->vals[i]);
    vec64_free(ui->grid);

    vec32_free(ui->workers.ops);
    vec_flow_free(ui->flows);
    htable_reset(&ui->index);

    SDL_DestroyTexture(ui->tex);
    free(ui);
}

static bool ui_factory_make_flow(
        struct ui_factory *ui, uint16_t index, struct chunk *chunk, im_id id)
{
    struct flow *flow = &ui->flows->vals[index];
    flow->tape_len = 0;

    const void *state = chunk_get(chunk, id);
    assert(state);

    const struct im_config *config = im_config_assert(im_id_item(id));
    assert(config->im.flow);

    if (!config->im.flow(state, flow)) return false;

    flow->row = flow->rank;
    ui->grid = vec64_grow(ui->grid, flow->rank + 1U);
    while (vec64_len(ui->grid) < flow->rank + 1U)
        ui->grid = vec64_append(ui->grid, 0);

    struct vec16 **vec = (void *) &ui->grid->vals[flow->row];
    flow->col = vec16_len(*vec);
    *vec = vec16_append(*vec, index);

    struct htable_ret ret = htable_put(&ui->index, flow->id, (uintptr_t) flow);
    assert(ret.ok);

    return true;
}

static void ui_factory_update(void *state, struct proxy *proxy)
{
    struct ui_factory *ui = state;
    assert(!coord_is_nil(ui->star));

    for (size_t i = 0; i < vec64_len(ui->grid); ++i) {
        struct vec16 *row = (void *) ui->grid->vals[i];
        if (row) row->len = 0;
    }

    struct chunk *chunk = proxy_chunk(proxy, ui->star);
    if (!chunk) {
        if (ui->flows) ui->flows->len = 0;
        if (ui->workers.ops) ui->workers.ops->len = 0;
        htable_reset(&ui->index);
        return;
    }

    struct vec16 *ids = chunk_list_filter(chunk, im_list_factory);
    ui->flows = vec_flow_grow(ui->flows, ids->len);
    ui->flows->len = 0;

    htable_reset(&ui->index);
    htable_reserve(&ui->index, vec16_len(ids));

    for (size_t i = 0; i < vec16_len(ids); ++i) {
        if (ui_factory_make_flow(ui, ui->flows->len, chunk, ids->vals[i]))
            ui->flows->len++;
    }

    if (ui->workers.ops) vec32_free(ui->workers.ops);

    ui->workers = chunk_workers(chunk);
    ui->workers.ops = vec32_copy(ui->workers.ops);

    free(ids);
}

void ui_factory_show(struct coord star, im_id id)
{
    struct ui_factory *ui = ui_state(ui_view_factory);
    assert(!coord_is_nil(star));

    ui->star = star;
    ui->scale = scale_init();
    ui_factory_update(ui, render.proxy);

    ui->pos = (struct flow_pos) {0};
    if (id) { /* TODO: set ui->pos to id's position. */ }

    ui_show(ui_view_factory);
}

coord_scale ui_factory_scale(void)
{
    struct ui_factory *ui = ui_state(ui_view_factory);
    return ui->scale;
}

struct coord ui_factory_coord(void)
{
    struct ui_factory *ui = ui_state(ui_view_factory);
    return ui->star;
}


// -----------------------------------------------------------------------------
// coordinates
// -----------------------------------------------------------------------------

static struct flow_rect ui_factory_flow_rect(
        struct ui_factory *ui, size_t row, size_t col)
{
    const size_t rows = vec64_len(ui->grid);
    assert(row < rows);

    const size_t cols = vec16_len((void *) ui->grid->vals[row]);
    assert(col < cols);

    const int64_t left = -(((int64_t) cols * ui->total.w) / 2);
    const int64_t top = -(((int64_t) rows * ui->total.h) / 2);

    return (struct flow_rect) {
        .x = left + (col * ui->total.w),
        .y = top + (row * ui->total.h),
        .w = ui->total.w, .h = ui->total.h,
    };
}

static ssize_t ui_factory_row(struct ui_factory *ui, struct flow_pos pos)
{
    const ssize_t rows = vec64_len(ui->grid);
    const int32_t top = -((rows * ui->total.h) / 2);
    if (pos.y < top) return -1;

    ssize_t result = (pos.y - top) / ui->total.h;
    return result < rows ? result : -1;
}

static ssize_t ui_factory_col(struct ui_factory *ui, struct flow_pos pos)
{
    const ssize_t row = ui_factory_row(ui, pos);
    if (row < 0 || (size_t) row >= vec64_len(ui->grid)) return -1;

    const ssize_t cols = vec16_len((void *) ui->grid->vals[row]);
    const int32_t left = -((cols * ui->total.w) / 2);
    if (pos.x < left) return -1;

    ssize_t result = (pos.x - left) / ui->total.w;
    return result < cols ? result : -1;
}

static SDL_Point ui_factory_project_sdl_point(
        struct ui_factory *ui, struct flow_pos origin)
{
    SDL_Rect rect = render.rect;
    int64_t x = origin.x, y = origin.y;

    int64_t rel_x = scale_div(ui->scale, x - ui->pos.x);
    int64_t rel_y = scale_div(ui->scale, y - ui->pos.y);

    return (SDL_Point) {
        .x = i64_clamp(rel_x + rect.w / 2 + rect.x, INT_MIN, INT_MAX),
        .y = i64_clamp(rel_y + rect.h / 2 + rect.y, INT_MIN, INT_MAX),
    };
}

static SDL_Rect ui_factory_project_sdl_rect(
        struct ui_factory *ui, struct flow_rect origin)
{
    SDL_Point top = ui_factory_project_sdl_point(ui, (struct flow_pos) {
                .x = origin.x,
                .y = origin.y });

    SDL_Point bot = ui_factory_project_sdl_point(ui, (struct flow_pos) {
                .x = origin.x + origin.w,
                .y = origin.y + origin.h });

    return (SDL_Rect) {
        .x = top.x,
        .y = top.y,
        .w = bot.x - top.x,
        .h = bot.y - top.y,
    };
}

static struct flow_pos ui_factory_project_flow_pos(
        struct ui_factory *ui, SDL_Point origin)
{
    SDL_Rect rect = render.rect;
    int64_t x = origin.x, y = origin.y;

    int64_t rel_x = scale_mult(ui->scale, x - rect.x - rect.w / 2);
    int64_t rel_y = scale_mult(ui->scale, y - rect.y - rect.h / 2);

    return (struct flow_pos) {
        .x = i64_clamp(ui->pos.x + rel_x, INT32_MIN, INT32_MAX),
        .y = i64_clamp(ui->pos.y + rel_y, INT32_MIN, INT32_MAX),
    };
}

static struct flow_rect ui_factory_project_flow_rect(
        struct ui_factory *ui, SDL_Rect origin)
{
    struct flow_pos top = ui_factory_project_flow_pos(ui, (SDL_Point) {
                .x = origin.x,
                .y = origin.y });

    struct flow_pos bot = ui_factory_project_flow_pos(ui, (SDL_Point) {
                .x = origin.x + origin.w,
                .y = origin.y + origin.h });

    return (struct flow_rect) {
        .x = top.x,
        .y = top.y,
        .w = bot.x - top.x,
        .h = bot.y - top.y,
    };
}

static bool ui_factory_flow_intersect(struct flow_rect a, struct flow_rect b)
{
    if (legion_max(a.x, b.x) >= legion_min(a.x + a.w, b.x + b.w)) return false;
    if (legion_max(a.y, b.y) >= legion_min(a.y + a.h, b.y + b.h)) return false;
    return true;
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static void ui_factory_event_user(struct ui_factory *, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case ev_star_select: {
        struct coord star = coord_from_u64((uintptr_t) ev->user.data1);
        ui_factory_show(star, 0);
        return;
    }

    case ev_item_select: {
        im_id id = (uintptr_t) ev->user.data1;
        struct coord star = coord_from_u64((uintptr_t) ev->user.data2);
        ui_factory_show(star, id);
        return;
    }

    default: { return; }
    }
}

static struct flow *ui_factory_cursor_flow(struct ui_factory *ui)
{
    struct flow_pos pos = ui_factory_project_flow_pos(ui, ui_cursor_point());

    ssize_t row = ui_factory_row(ui, pos);
    ssize_t col = ui_factory_col(ui, pos);
    if (row < 0 || col < 0) return NULL;

    struct vec16* vec = (void *) ui->grid->vals[row];
    return &ui->flows->vals[vec->vals[col]];
}

static bool ui_factory_event_click(struct ui_factory *ui)
{
    struct flow *flow = ui_factory_cursor_flow(ui);
    if (!flow) return false;

    ui_item_show(flow->id, ui->star);
    return true;
}

static bool ui_factory_event(void *state, SDL_Event *ev)
{
    struct ui_factory *ui = state;

    if (ev->type == render.event)
        ui_factory_event_user(ui, ev);

    switch (ev->type)
    {

    case SDL_MOUSEWHEEL: {
        ui->scale = scale_inc(ui->scale, -ev->wheel.y);
        return false;
    }

    case SDL_MOUSEMOTION: {
        if (!ui->panning) return false;
        int64_t xrel = scale_mult(ui->scale, ev->motion.xrel);
        ui->pos.x = i64_clamp(ui->pos.x - xrel, INT32_MIN, INT32_MAX);

        int64_t yrel = scale_mult(ui->scale, ev->motion.yrel);
        ui->pos.y = i64_clamp(ui->pos.y - yrel, INT32_MIN, INT32_MAX);

        ui->panned = true;
        return false;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_MouseButtonEvent *b = &ev->button;
        if (b->button != SDL_BUTTON_LEFT) return false;

        ui->panning = true;
        return false;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_MouseButtonEvent *b = &ev->button;
        if (b->button != SDL_BUTTON_LEFT) return false;

        ui->panning = false;
        if (ui->panned) { ui->panned = false; return false; }

        return ui_factory_event_click(ui);
    }

    default: { return false; }
    }
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void ui_factory_render_flow(
        struct ui_factory *ui,
        struct flow *flow,
        bool select,
        SDL_Renderer *renderer)
{
    sdl_err(SDL_SetRenderTarget(renderer, ui->tex));
    sdl_err(SDL_SetTextureBlendMode(ui->tex, SDL_BLENDMODE_BLEND));

    SDL_Rect rect = {
        .x = ui->pad.w, .y = ui->pad.h,
        .w = ui->inner.w + 2*ui->margin.w,
        .h = ui->inner.h + 2*ui->margin.h,
    };

    rgba_render(rgba_gray(select ? 0x22 : 0x11), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    rgba_render(rgba_gray(0x44), renderer);
    sdl_err(SDL_RenderDrawRect(renderer, &rect));

    struct ui_layout layout = ui_layout_new(
            make_pos(rect.x + ui->margin.w, rect.y + ui->margin.h),
            make_dim(ui->inner.w, ui->inner.h));

    ui_str_set_id(&ui->ui_id.str, flow->id);
    ui_label_render(&ui->ui_id, &layout, renderer);
    ui_layout_next_row(&layout);

    ui_str_set_item(&ui->ui_target.str, flow->target);
    ui_label_render(&ui->ui_target, &layout, renderer);
    if (flow->loops != im_loops_inf) {
        ui_label_render(&ui->ui_loops, &layout, renderer);
        ui_str_set_u64(&ui->ui_loops_val.str, flow->loops);
        ui_label_render(&ui->ui_loops_val, &layout, renderer);
    }
    ui_layout_next_row(&layout);

    switch (flow->state)
    {
    case tape_input: {
        ui_str_set_item(&ui->ui_tape_in.str, flow->item);
        ui_label_render(&ui->ui_tape_in, &layout, renderer);
        break;
    }
    case tape_work: {
        ui_label_render(&ui->ui_tape_work, &layout, renderer);
        break;
    }
    case tape_output: {
        ui_str_set_item(&ui->ui_tape_out.str, flow->item);
        ui_label_render(&ui->ui_tape_out, &layout, renderer);
        break;
    }
    case tape_eof: { break; }
    default: { assert(false); }
    }

    if (im_id_item(flow->id) == item_port &&
            flow->state == tape_output &&
            flow->tape_len)
    {
        ui_str_set_u64(&ui->ui_tape_num.str, flow->tape_len);
        ui_label_render(&ui->ui_tape_num, &layout, renderer);
    }
    else if (flow->state != tape_eof && flow->tape_len) {
        ui_str_set_u64(&ui->ui_tape_num.str, flow->tape_it+1);
        ui_label_render(&ui->ui_tape_num, &layout, renderer);

        ui_label_render(&ui->ui_tape_of, &layout, renderer);

        ui_str_set_u64(&ui->ui_tape_num.str, flow->tape_len);
        ui_label_render(&ui->ui_tape_num, &layout, renderer);
    }

    sdl_err(SDL_SetRenderTarget(renderer, NULL));
}

static void ui_factory_render_op(
        struct ui_factory *ui, uint32_t op, SDL_Renderer *renderer)
{
    struct htable_ret ret = {0};

    im_id src_id = 0, dst_id = 0;
    chunk_workers_ops(op, &src_id, &dst_id);

    ret = htable_get(&ui->index, src_id);
    struct flow *src = (struct flow *) ret.value;
    assert(ret.ok);

    ret = htable_get(&ui->index, dst_id);
    struct flow *dst = (struct flow *) ret.value;
    assert(ret.ok);

    struct flow_rect src_rect = ui_factory_flow_rect(ui, src->row, src->col);
    struct flow_rect dst_rect = ui_factory_flow_rect(ui, dst->row, dst->col);

    SDL_Point out = ui_factory_project_sdl_point(ui, (struct flow_pos) {
                .x = src_rect.x + ui->out.w,
                .y = src_rect.y + ui->out.h, });

    SDL_Point in = ui_factory_project_sdl_point(ui, (struct flow_pos) {
                .x = dst_rect.x + ui->in.w,
                .y = dst_rect.y + ui->in.h, });

    rgba_render(rgba_gray(0x88), renderer);
    sdl_err(SDL_RenderDrawLine(renderer, out.x, out.y, in.x, in.y));
}


static void ui_factory_render(
        void *state, struct ui_layout *, SDL_Renderer *renderer)
{
    struct ui_factory *ui = state;

    rgba_render(rgba_black(), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &render.rect));

    struct flow *cursor = ui_factory_cursor_flow(ui);
    struct flow_rect view = ui_factory_project_flow_rect(ui, render.rect);

    for (size_t i = 0; i < vec64_len(ui->grid); ++i) {
        struct vec16 *vec = (void *) ui->grid->vals[i];

        for (size_t j = 0; j < vec16_len(vec); ++j) {
            struct flow *flow = &ui->flows->vals[vec->vals[j]];

            struct flow_rect rect = ui_factory_flow_rect(ui, i, j);
            if (!ui_factory_flow_intersect(view, rect)) continue;

            SDL_Rect sdl = ui_factory_project_sdl_rect(ui, rect);
            ui_factory_render_flow(ui, flow, flow == cursor, renderer);
            sdl_err(SDL_RenderCopy(renderer, ui->tex, &ui->tex_rect, &sdl));
        }
    }

    for (size_t i = 0; i < vec32_len(ui->workers.ops); ++i) {
        uint32_t op = ui->workers.ops->vals[i];
        ui_factory_render_op(ui, op, renderer);
    }
}
