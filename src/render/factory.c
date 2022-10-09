/* factory.c
   Rémi Attab (remi.attab@gmail.com), 02 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/chunk.h"
#include "game/coord.h"
#include "items/io.h"
#include "items/item.h"
#include "items/types.h"
#include "render/ui.h"
#include "utils/vec.h"
#include "utils/htable.h"


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

struct factory
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

    bool active;
    bool panning, panned;
};

struct factory *factory_new(void)
{
    struct factory *factory = calloc(1, sizeof (*factory));
    *factory = (struct factory) {
        .active = false,

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

    factory->total = make_dim(
            factory->inner.w + 2*factory->margin.w + 2*factory->pad.w,
            factory->inner.h + 2*factory->margin.h + 2*factory->pad.h);

    factory->in = make_dim(
            factory->pad.w + factory->margin.w + factory->inner.w / 2,
            factory->pad.h);

    factory->out = make_dim(
            factory->pad.w + factory->margin.w + factory->inner.w / 2,
            factory->pad.h + 2*factory->margin.h + factory->inner.h);


    factory->tex_rect = (SDL_Rect) {
        .x = 0, .y = 0,
        .w = factory->total.w,
        .h = factory->total.h,
    };

    factory->tex = sdl_ptr(SDL_CreateTexture(
                    render.renderer,
                    SDL_PIXELFORMAT_RGBA8888,
                    SDL_TEXTUREACCESS_TARGET,
                    factory->tex_rect.w, factory->tex_rect.h));

    return factory;
}

void factory_free(struct factory *factory)
{
    ui_label_free(&factory->ui_id);
    ui_label_free(&factory->ui_target);
    ui_label_free(&factory->ui_loops);
    ui_label_free(&factory->ui_loops_val);
    ui_label_free(&factory->ui_tape_in);
    ui_label_free(&factory->ui_tape_work);
    ui_label_free(&factory->ui_tape_out);
    ui_label_free(&factory->ui_tape_num);
    ui_label_free(&factory->ui_tape_of);

    for (size_t i = 0; i < vec64_len(factory->grid); ++i)
        vec16_free((struct vec16 *) factory->grid->vals[i]);
    vec64_free(factory->grid);

    vec32_free(factory->workers.ops);
    vec_flow_free(factory->flows);
    htable_reset(&factory->index);

    SDL_DestroyTexture(factory->tex);
    free(factory);
}


bool factory_active(struct factory *factory)
{
    return factory->active;
}

coord_scale factory_scale(struct factory *factory)
{
    return factory->scale;
}

struct coord factory_coord(struct factory *factory)
{
    return factory->star;
}


// -----------------------------------------------------------------------------
// coordinates
// -----------------------------------------------------------------------------

static struct flow_rect factory_flow_rect(
        struct factory *factory, size_t row, size_t col)
{
    const size_t rows = vec64_len(factory->grid);
    assert(row < rows);

    const size_t cols = vec16_len((void *) factory->grid->vals[row]);
    assert(col < cols);

    const int64_t left = -(((int64_t) cols * factory->total.w) / 2);
    const int64_t top = -(((int64_t) rows * factory->total.h) / 2);

    return (struct flow_rect) {
        .x = left + (col * factory->total.w),
        .y = top + (row * factory->total.h),
        .w = factory->total.w, .h = factory->total.h,
    };
}

static ssize_t factory_row(struct factory *factory, struct flow_pos pos)
{
    const ssize_t rows = vec64_len(factory->grid);
    const int32_t top = -((rows * factory->total.h) / 2);
    if (pos.y < top) return -1;

    ssize_t result = (pos.y - top) / factory->total.h;
    return result < rows ? result : -1;
}

static ssize_t factory_col(struct factory *factory, struct flow_pos pos)
{
    const ssize_t row = factory_row(factory, pos);
    if (row < 0 || (size_t) row >= vec64_len(factory->grid)) return -1;

    const ssize_t cols = vec16_len((void *) factory->grid->vals[row]);
    const int32_t left = -((cols * factory->total.w) / 2);
    if (pos.x < left) return -1;

    ssize_t result = (pos.x - left) / factory->total.w;
    return result < cols ? result : -1;
}

static SDL_Point factory_project_sdl_point(
        struct factory *factory, struct flow_pos origin)
{
    SDL_Rect rect = render.rect;
    int64_t x = origin.x, y = origin.y;

    int64_t rel_x = scale_div(factory->scale, x - factory->pos.x);
    int64_t rel_y = scale_div(factory->scale, y - factory->pos.y);

    return (SDL_Point) {
        .x = i64_clamp(rel_x + rect.w / 2 + rect.x, INT_MIN, INT_MAX),
        .y = i64_clamp(rel_y + rect.h / 2 + rect.y, INT_MIN, INT_MAX),
    };
}

static SDL_Rect factory_project_sdl_rect(
        struct factory *factory, struct flow_rect origin)
{
    SDL_Point top = factory_project_sdl_point(factory, (struct flow_pos) {
                .x = origin.x,
                .y = origin.y });

    SDL_Point bot = factory_project_sdl_point(factory, (struct flow_pos) {
                .x = origin.x + origin.w,
                .y = origin.y + origin.h });

    return (SDL_Rect) {
        .x = top.x,
        .y = top.y,
        .w = bot.x - top.x,
        .h = bot.y - top.y,
    };
}

static struct flow_pos factory_project_flow_pos(
        struct factory *factory, SDL_Point origin)
{
    SDL_Rect rect = render.rect;
    int64_t x = origin.x, y = origin.y;

    int64_t rel_x = scale_mult(factory->scale, x - rect.x - rect.w / 2);
    int64_t rel_y = scale_mult(factory->scale, y - rect.y - rect.h / 2);

    return (struct flow_pos) {
        .x = i64_clamp(factory->pos.x + rel_x, INT32_MIN, INT32_MAX),
        .y = i64_clamp(factory->pos.y + rel_y, INT32_MIN, INT32_MAX),
    };
}

static struct flow_rect factory_project_flow_rect(
        struct factory *factory, SDL_Rect origin)
{
    struct flow_pos top = factory_project_flow_pos(factory, (SDL_Point) {
                .x = origin.x,
                .y = origin.y });

    struct flow_pos bot = factory_project_flow_pos(factory, (SDL_Point) {
                .x = origin.x + origin.w,
                .y = origin.y + origin.h });

    return (struct flow_rect) {
        .x = top.x,
        .y = top.y,
        .w = bot.x - top.x,
        .h = bot.y - top.y,
    };
}

static bool factory_flow_intersect(struct flow_rect a, struct flow_rect b)
{
    if (legion_max(a.x, b.x) >= legion_min(a.x + a.w, b.x + b.w)) return false;
    if (legion_max(a.y, b.y) >= legion_min(a.y + a.h, b.y + b.h)) return false;
    return true;
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static bool factory_make_flow(
        struct factory *factory, uint16_t index, struct chunk *chunk, im_id id)
{
    struct flow *flow = &factory->flows->vals[index];
    flow->tape_len = 0;

    const void *state = chunk_get(chunk, id);
    assert(state);

    const struct im_config *config = im_config_assert(im_id_item(id));
    assert(config->im.flow);

    if (!config->im.flow(state, flow)) return false;

    flow->row = flow->rank - 1;
    factory->grid = vec64_grow(factory->grid, flow->rank);
    while (vec64_len(factory->grid) < flow->rank)
        factory->grid = vec64_append(factory->grid, 0);

    struct vec16 **vec = (void *) &factory->grid->vals[flow->row];
    flow->col = vec16_len(*vec);
    *vec = vec16_append(*vec, index);

    struct htable_ret ret = htable_put(&factory->index, flow->id, (uintptr_t) flow);
    assert(ret.ok);

    return true;
}

static void factory_update(struct factory *factory)
{
    if (!factory->active) return;
    assert(!coord_is_nil(factory->star));

    for (size_t i = 0; i < vec64_len(factory->grid); ++i) {
        struct vec16 *row = (void *) factory->grid->vals[i];
        if (row) row->len = 0;
    }

    struct chunk *chunk = proxy_chunk(render.proxy, factory->star);
    if (!chunk) {
        if (factory->flows) factory->flows->len = 0;
        if (factory->workers.ops) factory->workers.ops->len = 0;
        htable_reset(&factory->index);
        return;
    }

    struct vec16 *ids = chunk_list_filter(chunk, im_list_factory);
    factory->flows = vec_flow_grow(factory->flows, ids->len);
    factory->flows->len = 0;

    htable_reset(&factory->index);
    htable_reserve(&factory->index, vec16_len(ids));

    for (size_t i = 0; i < vec16_len(ids); ++i) {
        if (factory_make_flow(factory, factory->flows->len, chunk, ids->vals[i]))
            factory->flows->len++;
    }

    if (factory->workers.ops) vec32_free(factory->workers.ops);

    factory->workers = chunk_workers(chunk);
    factory->workers.ops = vec32_copy(factory->workers.ops);

    free(ids);
}

static void factory_event_star(struct factory *factory, struct coord star)
{
    factory->scale = scale_init();
    factory->star = star;
    factory->pos = (struct flow_pos) {0};
    factory_update(factory);
}

static bool factory_event_user(struct factory *factory, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_STATE_LOAD: {
        factory->active = false;
        return false;
    }

    case EV_STATE_UPDATE: {
        if (!factory->active) return false;
        factory_update(factory);
        return false;
    }

    case EV_MAP_GOTO:
    case EV_FACTORY_CLOSE: { factory->active = false; return false; }

    case EV_FACTORY_SELECT: {
        factory->active = true;
        factory_event_star(factory, coord_from_u64((uintptr_t) ev->user.data1));
        return false;
    }

    case EV_STAR_SELECT: {
        if (!factory->active) return false;
        factory_event_star(factory, coord_from_u64((uintptr_t) ev->user.data1));
        return false;
    }

    case EV_ITEM_SELECT: {
        if (!factory->active) return false;
        factory_event_star(factory, coord_from_u64((uintptr_t) ev->user.data2));
        return false;
    }

    default: { return false; }
    }
}

static struct flow *factory_cursor_flow(struct factory *factory)
{
    struct flow_pos pos = factory_project_flow_pos(factory, render.cursor.point);

    ssize_t row = factory_row(factory, pos);
    ssize_t col = factory_col(factory, pos);
    if (row < 0 || col < 0) return NULL;

    struct vec16* vec = (void *) factory->grid->vals[row];
    return &factory->flows->vals[vec->vals[col]];
}

static bool factory_event_click(struct factory *factory)
{
    struct flow *flow = factory_cursor_flow(factory);
    if (!flow) return false;

    render_push_event(EV_ITEM_SELECT, flow->id, coord_to_u64(factory->star));
    return true;
}

bool factory_event(struct factory *factory, SDL_Event *ev)
{
    if (ev->type == render.event && factory_event_user(factory, ev)) return true;
    if (!factory->active) return false;

    switch (ev->type)
    {

    case SDL_MOUSEWHEEL: {
        factory->scale = scale_inc(factory->scale, -ev->wheel.y);
        return false;
    }

    case SDL_MOUSEMOTION: {
        if (!factory->panning) return false;
        int64_t xrel = scale_mult(factory->scale, ev->motion.xrel);
        factory->pos.x = i64_clamp(factory->pos.x - xrel, INT32_MIN, INT32_MAX);

        int64_t yrel = scale_mult(factory->scale, ev->motion.yrel);
        factory->pos.y = i64_clamp(factory->pos.y - yrel, INT32_MIN, INT32_MAX);

        factory->panned = true;
        return false;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_MouseButtonEvent *b = &ev->button;
        if (b->button != SDL_BUTTON_LEFT) return false;

        factory->panning = true;
        return false;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_MouseButtonEvent *b = &ev->button;
        if (b->button != SDL_BUTTON_LEFT) return false;

        factory->panning = false;
        if (factory->panned) { factory->panned = false; return false; }

        return factory_event_click(factory);
    }

    default: { return false; }
    }
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void factory_render_flow(
        struct factory *factory,
        struct flow *flow,
        bool select,
        SDL_Renderer *renderer)
{
    sdl_err(SDL_SetRenderTarget(renderer, factory->tex));
    sdl_err(SDL_SetTextureBlendMode(factory->tex, SDL_BLENDMODE_BLEND));

    SDL_Rect rect = {
        .x = factory->pad.w, .y = factory->pad.h,
        .w = factory->inner.w + 2*factory->margin.w,
        .h = factory->inner.h + 2*factory->margin.h,
    };

    rgba_render(rgba_gray(select ? 0x22 : 0x11), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    rgba_render(rgba_gray(0x44), renderer);
    sdl_err(SDL_RenderDrawRect(renderer, &rect));

    struct ui_layout layout = ui_layout_new(
            make_pos(rect.x + factory->margin.w, rect.y + factory->margin.h),
            make_dim(factory->inner.w, factory->inner.h));

    ui_str_set_id(&factory->ui_id.str, flow->id);
    ui_label_render(&factory->ui_id, &layout, renderer);
    ui_layout_next_row(&layout);

    ui_str_set_item(&factory->ui_target.str, flow->target);
    ui_label_render(&factory->ui_target, &layout, renderer);
    if (flow->loops != im_loops_inf) {
        ui_label_render(&factory->ui_loops, &layout, renderer);
        ui_str_set_u64(&factory->ui_loops_val.str, flow->loops);
        ui_label_render(&factory->ui_loops_val, &layout, renderer);
    }
    ui_layout_next_row(&layout);

    switch (flow->state)
    {
    case tape_input: {
        ui_str_set_item(&factory->ui_tape_in.str, flow->item);
        ui_label_render(&factory->ui_tape_in, &layout, renderer);
        break;
    }
    case tape_work: {
        ui_label_render(&factory->ui_tape_work, &layout, renderer);
        break;
    }
    case tape_output: {
        ui_str_set_item(&factory->ui_tape_out.str, flow->item);
        ui_label_render(&factory->ui_tape_out, &layout, renderer);
        break;
    }
    case tape_eof: { break; }
    default: { assert(false); }
    }

    if (im_id_item(flow->id) == ITEM_PORT &&
            flow->state == tape_output &&
            flow->tape_len)
    {
        ui_str_set_u64(&factory->ui_tape_num.str, flow->tape_len);
        ui_label_render(&factory->ui_tape_num, &layout, renderer);
    }
    else if (flow->state != tape_eof && flow->tape_len) {
        ui_str_set_u64(&factory->ui_tape_num.str, flow->tape_it+1);
        ui_label_render(&factory->ui_tape_num, &layout, renderer);

        ui_label_render(&factory->ui_tape_of, &layout, renderer);

        ui_str_set_u64(&factory->ui_tape_num.str, flow->tape_len);
        ui_label_render(&factory->ui_tape_num, &layout, renderer);
    }

    sdl_err(SDL_SetRenderTarget(renderer, NULL));
}

static void factory_render_op(
        struct factory *factory, uint32_t op, SDL_Renderer *renderer)
{
    struct htable_ret ret = {0};

    im_id src_id = 0, dst_id = 0;
    chunk_workers_ops(op, &src_id, &dst_id);

    ret = htable_get(&factory->index, src_id);
    struct flow *src = (struct flow *) ret.value;
    assert(ret.ok);

    ret = htable_get(&factory->index, dst_id);
    struct flow *dst = (struct flow *) ret.value;
    assert(ret.ok);

    struct flow_rect src_rect = factory_flow_rect(factory, src->row, src->col);
    struct flow_rect dst_rect = factory_flow_rect(factory, dst->row, dst->col);

    SDL_Point out = factory_project_sdl_point(factory, (struct flow_pos) {
                .x = src_rect.x + factory->out.w,
                .y = src_rect.y + factory->out.h, });

    SDL_Point in = factory_project_sdl_point(factory, (struct flow_pos) {
                .x = dst_rect.x + factory->in.w,
                .y = dst_rect.y + factory->in.h, });

    rgba_render(rgba_gray(0x88), renderer);
    sdl_err(SDL_RenderDrawLine(renderer, out.x, out.y, in.x, in.y));
}


void factory_render(struct factory *factory, SDL_Renderer *renderer)
{
    if (!factory->active) return;

    rgba_render(rgba_black(), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &render.rect));

    struct flow *cursor = factory_cursor_flow(factory);
    struct flow_rect view = factory_project_flow_rect(factory, render.rect);

    for (size_t i = 0; i < vec64_len(factory->grid); ++i) {
        struct vec16 *vec = (void *) factory->grid->vals[i];

        for (size_t j = 0; j < vec16_len(vec); ++j) {
            struct flow *flow = &factory->flows->vals[vec->vals[j]];

            struct flow_rect rect = factory_flow_rect(factory, i, j);
            if (!factory_flow_intersect(view, rect)) continue;

            SDL_Rect sdl = factory_project_sdl_rect(factory, rect);
            factory_render_flow(factory, flow, flow == cursor, renderer);
            sdl_err(SDL_RenderCopy(renderer, factory->tex, &factory->tex_rect, &sdl));
        }
    }

    for (size_t i = 0; i < vec32_len(factory->workers.ops); ++i) {
        uint32_t op = factory->workers.ops->vals[i];
        factory_render_op(factory, op, renderer);
    }
}
