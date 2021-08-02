/* factory.c
   Rémi Attab (remi.attab@gmail.com), 02 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "render/factory.h"

// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

struct box
{
    id_t id;
    uint16_t row, col;
    loops_t loops;
    enum item in, out;
    enum item target;
};

#define vecx_type struct box
#define vecx_name vec_box
#include "utils/vecx.h"

#define vecx_type uint16_t
#define vecx_name vec16
#include "utils/vecx.h"


// -----------------------------------------------------------------------------
// factory
// -----------------------------------------------------------------------------

struct factory
{
    struct coord star;

    struct vec64 *grid;
    struct vec_box *boxes;
    struct htable index;
    struct workers workers;

    scale_t scale;
    struct pos pos;
    struct dim inner, margin, pad, total, in, out;

    SDL_Trxture *tex;
    SDL_Rect tex_rect;
    struct ui_label ui_id, ui_item, ui_loops, ui_sep;

    bool active;
    bool panning, panned;
};

static struct font *factory_font(void) { return font_mono6; }

struct factory *factory_new(void)
{
    struct font *font = factory_font();
    struct factory *factory = calloc(1, sizeof (*factory));

    factory->ui_id = ui_label_new(font, ui_str_v(id_str_len));
    factory->ui_item = ui_label_new(font, ui_str_v(item_str_len));
    factory->ui_loops = ui_label_new(font, ui_str_v(3));
    factory->ui_sep = ui_label_new(font, ui_str_c(" x"));

    factory->scale = scale_init();

    factory->inner = make_dim(id_str_len * font->glyph_w, 3 * font->glyph_h);
    factory->margin = make_dim(5, 5);
    factory->pad = make_dim(20, 20);
    factory->total = make_dim(
            factory->inner.w + factory->margin.w + factory->pad.w,
            factory->inner.h + factory->margin.h + factory->pad.h);

    factory->in = make_dim(
            factory->pad.w + factory->margin.w + factory->inner.w / 2,
            factory->pad.h + factory->margin.h);

    factory->out = make_dim(
            factory->pad.w + factory->margin.w + factory->inner.w / 2,
            factory->pad.h + factory->margin.h + factory->inner.h);


    factory->tex_rect = (SDL_Rect) {
        .x = 0, .y = 0,
        .w = factory->total.w,
        .h = factory->total.h,
    };

    factory->tex = sdl_ptr(SDL_CreateTexture(
                    core.renderer,
                    SDL_PIXELFORMAT_RGBA8888,
                    SDL_TEXTUREACCESS_TARGET,
                    factory->tex_rect.w, factory->tex_rect.h));


    return factory;
}

void factory_free(struct factory *factory)
{
    for (size_t i = 0; i < vec64_len(factory->grid); ++i)
        vec16_free((struct vec16 *) factory->grid->vals[i]);
    vec64_free(factory->grid);
    vec_box_free(factory->boxes);
    free(factory);
}

// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static bool factory_make_box(
        struct factory *factory, uint16_t index, struct chunk *chunk, id_t id)
{
    struct box *box = &factory->boxes[index];

    switch (id_item(id))
    {

    case ITEM_DEPLOY: {
        struct deploy *deploy = chunk_get(chunk, id);
        if (!deploy->item) return false;
        *box = (struct box) {
            .id = deploy->id,
            .loops = deploy->loops,
            .target = deploy->item,
            .in = deploy->item,
        };
        break;
    }

    case ITEM_STORAGE: {
        struct storage *storage = chunk_get(chunk, id);
        if (!storage->item) return false;
        *box = (struct box) {
            .id = storage->id,
            .loops = storage->count,
            .target = storage->item,
            .in = storage->item,
        };
        break;
    }


    case ITEM_EXTRACT_1...ITEM_EXTRACT_3: {
        struct extract *extract = chunk_get(chunk, id);
        if (!extract->tape) return false;

        *box = (struct box) {
            .id = extract->id,
            .loops = extract->loops,
            .target = tape_packed_id(extract->tape);
        };

        const struct tape *tape = tape_packed_ptr(extract->tape);
        struct tape_ret ret = tape_at(tape, tape_packed_it(extract->tape));

        switch (ret.state) {
        case tape_input: { box->in = ret.item; break; }
        case tape_output: { box->out = ret.item; break; }
        }

        break;
    }

    case ITEM_PRINTER_1...ITEM_ASSEMBLY_3: {
        struct printer *printer = chunk_get(chunk, id);
        if (!printer->tape) return false;

        *box = (struct box) {
            .id = printer->id,
            .loops = printer->loops,
            .target = tape_packed_id(printer->tape);
        };

        const struct tape *tape = tape_packed_ptr(printer->tape);
        struct tape_ret ret = tape_at(tape, tape_packed_it(printer->tape));

        switch (ret.state) {
        case tape_input: { box->in = ret.item; break; }
        case tape_output: { box->out = ret.item; break; }
        }

        break;
    }

    default: { assert(false); }
    }

    box->row = tape_stats(box->target)->rank - 1;

    factory->grid = vec64_grow(factory->grid, box->row + 1);
    while (vec64_len(factory->grid) < box->row + 1)
        factory->grid = vec64_append(factory->grid, 0);

    struct vec16 **vec = (void **) &factory->grid->vals[box->row];
    box->col = (*vec)->len;
    *vec = vec16_append(index);

    struct htable_ret ret = htable_put(&factory->index, box->target, box);
    assert(ret.ok);

    return true;
}

static void factory_update(struct factory *factory)
{
    assert(!coord_is_nil(factory->coord));

    struct chunk *chunk = world_chunk(core.state.world, factory->coord);
    assert(chunk);

    factory->boxes->len = 0;
    for (size_t i = 0; i < vec64_len(factory->grid); ++i) {
        struct vec16 *row = (void *) factory->grid.vals[i];
        if (row) row->len = 0;
    }

    static const enum item filter[] = {
        ITEM_DEPLOY, ITEM_STORAGE,
        ITEM_EXTRACT_1, ITEM_EXTRACT_2, ITEM_EXTRACT_3,
        ITEM_PRINTER_1, ITEM_PRINTER_2, ITEM_PRINTER_3,
        ITEM_ASSEMBLY_1, ITEM_ASSEMBLY_2, ITEM_ASSEMBLY_3,
    };
    struct vec64 *ids = chunk_list_filter(chunk, filter, array_len(filter));
    factory->boxes = vec_box_grow(factory->boxes, ids->len);

    htable_reset(&factory->index);
    htable_reserve(&factory->index, vec64_len(ids));

    for (size_t i = 0; i < vec64_len(ids); ++i) {
        struct box *box = factory->boxes->vals[factory->boxes->len];
        if (factory_make_box(factory, box, chunk, ids->vals[i]))
            factory->boxes->len++;
    }

    factory->workers = chunk_workers(chunk);
    factory->workers.ops = vec64_copy(factory->workers.ops);

    free(ids);
}

static bool factory_event_user(struct factory *factory, SDL_Event *ev)
{
    switch (ev->user.code)
    {

    case EV_MAP_GOTO:
    case EV_FACTORY_CLOSE: { factory->active = false; break; }

    case EV_FACTORY_SELECT: {
        factory->active = true;
        factory->coord = id_to_coord((uintptr_t) ev->user.data1);
        factory_update(factory);
        break;
    }

    case EV_STATE_UPDATE: { factory_update(factory); break; }

    default: { return false; }
    }
}

bool factory_event(struct factory *factory, SDL_Event *ev)
{
    if (ev->type == core.event) return factory_event_user(factory, ev);

    switch (ev->type)
    {

    case SDL_MOUSEWHEEL: {
        factory->scale = scale_inc(factory->scale, -ev->wheel.y);
        break;
    }

    case SDL_MOUSEMOTION: {
        if (factory->panning) {
            int64_t xrel = scale_mult(factory->scale, ev->motion.xrel);
            factory->pos.x = i64_clamp(factory->pos.x - xrel, 0, UINT32_MAX);

            int64_t yrel = scale_mult(factory->scale, ev->motion.yrel);
            factory->pos.y = i64_clamp(factory->pos.y - yrel, 0, UINT32_MAX);

            factory->panned = true;
        }
        break;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_MouseButtonEv *b = &ev->button;
        if (b->button != SDL_BUTTON_LEFT) break;
        factory->panning = true;
        break;
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_MouseButtonEv *b = &ev->button;
        if (b->button != SDL_BUTTON_LEFT) break;
        factory->panning = false;
        factory->panned = false;
        break;
    }

    default: { return false; }
    }
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void factory_render_box(
        struct factory *factory, struct box *box, SDL_Renderer *renderer)
{
    sdl_err(SDL_SetRenderTarget(renderer, factory->tex));
    sdl_err(SDL_SetTextureBlendMode(factory->tex, SDL_BLENDMODE_BLEND));

    SDL_Rect rect = {
        .x = factory->pad.w, .y = factory->pad.h,
        .w = factory->inner.w + 2*factory->margin.w,
        .h = factory->inner.h + 2*factory->margin.h,
    };

    rgba_render(rgba_gray(0x11), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &rect));

    rgba_render(rgba_gray(0x44), renderer);
    sdl_err(SDL_RenderDrawRect(renderer, &rect));

    struct layout layout = ui_layout_new(
            make_pos(rect.x + factory->margin.w, rect.y + factory->margin.h),
            make_dim(factory->inner.w, factory->inner.h));

    ui_str_set_id(&factory->ui_id, box->id);
    ui_label_render(&factory->ui_id, &layout, renderer);
    layout_next_row(&layout);

    ui_str_set_item(&factory->ui_item, box->item);
    ui_label_render(&factory->ui_tem, &layout, renderer);
    ui_label_render(&factory->sep, &layout, renderer);
    ui_str_set_u64(&factory->ui_loops, box->loops);
    ui_label_render(&factory->loops, &layout, renderer);
    layout_next_row(&layout);

    if (box->in) {
        factory->ui_item.fg = rgba_green();
        ui_str_set_item(&factory->ui_item, box->in);
        ui_label_render(&factory->ui_item, &layout, renderer);
    }
    else if (box->out) {
        factory->ui_item.fg = rgba_blue();
        ui_str_set_item(&factory->ui_item, box->out);
        ui_label_render(&factory->ui_item, &layout, renderer);
    }

    sdl_err(SDL_SetRenderTarget(renderer, NULL));
}

static void factory_render_ops(
        struct factory *factory, uint64_t op, SDL_Renderer *renderer)
{
    struct htable_ret ret = {0};

    id_t src_id = 0, dst_id = 0;
    chunk_worker_ops(op, &src_id, &dst_id);

    ret = htable_get(&factory->index, src_id);
    struct box *src = (struct box *) ret.value;
    assert(ret.ok);

    ret = htable_get(&factory->index, dst_id);
    struct box *dst = (struct box *) ret.value;
    assert(ret.ok);

    struct pos src_pos = make_pos(
            src->row*factory->total.w + factory->out.w - factory->pos.x,
            src->col*factory->total.h + factory->out.h - factory->pos.y);

    struct pos dst_pos = make_pos(
            dst->row*factory->total.w + factory->in.w - factory->pos.x,
            dst->col*factory->total.h + factory->in.h - factory->pos.y);

    rbga_render(rgba_white(), renderer);
    sdl_err(SDL_DrawLine(renderer, src_pos.x, src_pos.y, dst_pos.x, dst_pos.y));
}


void factory_render(struct factory *factory, SDL_Renderer *renderer)
{
    struct font *font = factory_font();

    size_t rows = i64_ceil_div(core.rect.w, factory->total.w);
    size_t cols = i64_ceil_div(core.rect.h, factory->total.h);

    size_t row = factory->pos.x / factory->total.w;
    size_t col = factory->pos.y / factory->total.h;

    for (size_t i = row; i < rows && i < vec64_len(factory->grid); ++i) {
        struct vec16 *vec = (void *) factory->grid->vals[i];

        for (size_t j = col; j < cols && j < vec16_len(vec); ++j) {
            struct box *box = &factory->boxes[vec->vals[j]];
            factory_render_box(factory, box, renderer);

            SDL_Rect rect = {
                .x = i*factory->total.w - factory->pos.x,
                .y = j*factory->total.h - factory->pos.y,
                .w = factory->total.w,
                .h = factory->total.h,
            };
            sdl_err(SDL_RenderCopy(renderer, factory->tex, &factory->tex_rect, &rect));
        }
    }

    for (size_t i = 0; i < vec64_len(factory->workers.ops); ++i) {
        uint64_t op = factory->workers.ops->vals[i];
        factory_render_op(factory, op, renderer);
    }
}
