/* factory.c
   Rémi Attab (remi.attab@gmail.com), 02 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/io.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/coord.h"
#include "game/items/items.h"
#include "render/ui.h"
#include "utils/vec.h"
#include "utils/htable.h"

// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

struct legion_packed box
{
    id_t id;
    uint16_t row, col;

    loops_t loops;
    enum item target;

    legion_pad(1);

    enum item in, out;
    tape_it_t tape_it, tape_len;
};

static_assert(sizeof(struct box) == 16);

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

    struct pos pos;
    struct dim inner, margin, pad, total, in, out;

    SDL_Texture *tex;
    SDL_Rect tex_rect;

    struct ui_label ui_id;
    struct ui_label ui_target, ui_loops, ui_loops_val;
    struct ui_label ui_tape_in, ui_tape_out, ui_tape_num, ui_tape_of;

    bool active;
    bool panning, panned;
};

static struct font *factory_font(void) { return font_mono6; }

struct factory *factory_new(void)
{
    struct font *font = factory_font();
    struct factory *factory = calloc(1, sizeof (*factory));

    *factory = (struct factory) {
        .active = false,

        .ui_id = ui_label_new(font, ui_str_v(id_str_len)),
        .ui_target = ui_label_new(font, ui_str_v(item_str_len)),
        .ui_loops = ui_label_new(font, ui_str_c("x")),
        .ui_loops_val = ui_label_new(font, ui_str_v(3)),
        .ui_tape_in = ui_label_new(font, ui_str_v(item_str_len)),
        .ui_tape_out = ui_label_new(font, ui_str_v(item_str_len)),
        .ui_tape_num = ui_label_new(font, ui_str_v(3)),
        .ui_tape_of = ui_label_new(font, ui_str_c("/")),

        .inner = make_dim(id_str_len * font->glyph_w, 3 * font->glyph_h),
        .margin = make_dim(5, 5),
        .pad = make_dim(20, 20),
    };

    factory->ui_tape_in.fg = rgba_green();
    factory->ui_tape_out.fg = rgba_blue();

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
                    core.renderer,
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
    ui_label_free(&factory->ui_tape_out);
    ui_label_free(&factory->ui_tape_num);
    ui_label_free(&factory->ui_tape_of);

    for (size_t i = 0; i < vec64_len(factory->grid); ++i)
        vec16_free((struct vec16 *) factory->grid->vals[i]);
    vec64_free(factory->grid);

    vec64_free(factory->workers.ops);
    vec_box_free(factory->boxes);
    htable_reset(&factory->index);

    SDL_DestroyTexture(factory->tex);
    free(factory);
}


// -----------------------------------------------------------------------------
// events
// -----------------------------------------------------------------------------

static bool factory_make_box(
        struct factory *factory, uint16_t index, struct chunk *chunk, id_t id)
{
    struct box *box = &factory->boxes->vals[index];
    box->tape_len = 0;

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
            .target = tape_packed_id(extract->tape),
        };

        const struct tape *tape = tape_packed_ptr(extract->tape);
        struct tape_ret ret = tape_at(tape, tape_packed_it(extract->tape));

        switch (ret.state) {
        case tape_input: { box->in = ret.item; break; }
        case tape_output: { box->out = ret.item; break; }
        default: { break; }
        }

        box->tape_it = tape_packed_it(extract->tape);
        box->tape_len = tape_len(tape);
        break;
    }

    case ITEM_PRINTER_1...ITEM_ASSEMBLY_3: {
        struct printer *printer = chunk_get(chunk, id);
        if (!printer->tape) return false;

        *box = (struct box) {
            .id = printer->id,
            .loops = printer->loops,
            .target = tape_packed_id(printer->tape),
        };

        const struct tape *tape = tape_packed_ptr(printer->tape);
        struct tape_ret ret = tape_at(tape, tape_packed_it(printer->tape));

        switch (ret.state) {
        case tape_input: { box->in = ret.item; break; }
        case tape_output: { box->out = ret.item; break; }
        default: { break; }
        }

        box->tape_it = tape_packed_it(printer->tape);
        box->tape_len = tape_len(tape);
        break;
    }

    case ITEM_RESEARCH: {
        struct research *research = chunk_get(chunk, id);
        if (!research->item) return false;
        *box = (struct box) {
            .id = research->id,
            .target = research->item,
            .in = research->state == research_waiting ? research->item : 0,
        };
        break;
    }

    default: { assert(false); }
    }

    size_t rank = tapes_stats(box->target)->rank;
    if (id_item(box->id) == ITEM_DEPLOY) rank++;
    box->row = rank - 1;

    factory->grid = vec64_grow(factory->grid, rank);
    while (vec64_len(factory->grid) < rank)
        factory->grid = vec64_append(factory->grid, 0);

    struct vec16 **vec = (void *) &factory->grid->vals[box->row];
    box->col = vec16_len(*vec);
    *vec = vec16_append(*vec, index);

    struct htable_ret ret = htable_put(&factory->index, box->id, (uintptr_t) box);
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

    struct chunk *chunk = world_chunk(core.state.world, factory->star);
    if (!chunk) {
        factory->boxes->len = 0;
        factory->workers.ops->len = 0;
        htable_reset(&factory->index);
        return;
    }

    static const enum item filter[] = {
        ITEM_DEPLOY, ITEM_STORAGE, ITEM_RESEARCH,
        ITEM_EXTRACT_1, ITEM_EXTRACT_2, ITEM_EXTRACT_3,
        ITEM_PRINTER_1, ITEM_PRINTER_2, ITEM_PRINTER_3,
        ITEM_ASSEMBLY_1, ITEM_ASSEMBLY_2, ITEM_ASSEMBLY_3,
    };
    struct vec64 *ids = chunk_list_filter(chunk, filter, array_len(filter));

    factory->boxes = vec_box_grow(factory->boxes, ids->len);
    factory->boxes->len = 0;

    htable_reset(&factory->index);
    htable_reserve(&factory->index, vec64_len(ids));

    for (size_t i = 0; i < vec64_len(ids); ++i) {
        if (factory_make_box(factory, factory->boxes->len, chunk, ids->vals[i]))
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
    case EV_FACTORY_CLOSE: { factory->active = false; return false; }

    case EV_FACTORY_SELECT: {
        factory->active = true;
        factory->star = id_to_coord((uintptr_t) ev->user.data1);
        factory->pos = make_pos(0, -20);
        factory_update(factory);
        return false;
    }

    case EV_STATE_LOAD: { factory->active = false; return false; }
    case EV_STATE_UPDATE: { factory_update(factory); return false; }

    default: { return false; }
    }
}

static struct box *factory_cursor_box(struct factory *factory)
{
    SDL_Point point = core.cursor.point;
    point.x += factory->pos.x;
    point.y += factory->pos.y;

    ssize_t row = point.y / factory->total.h;
    if (row < 0 || row >= (ssize_t) vec64_len(factory->grid)) return NULL;
    struct vec16* vec = (void *) factory->grid->vals[row];

    ssize_t col = point.x / factory->total.w;
    if (col < 0 || col >= (ssize_t) vec16_len(vec)) return NULL;
    struct box *box = &factory->boxes->vals[vec->vals[col]];

    return box;
}

static bool factory_event_click(struct factory *factory)
{
    struct box *box = factory_cursor_box(factory);
    if (!box) return false;

    core_push_event(EV_ITEM_SELECT, box->id, coord_to_id(factory->star));
    return true;
}

bool factory_event(struct factory *factory, SDL_Event *ev)
{
    if (ev->type == core.event && factory_event_user(factory, ev)) return true;
    if (!factory->active) return false;

    switch (ev->type)
    {

    case SDL_MOUSEMOTION: {
        if (!factory->panning) return false;
        factory->pos.x -= ev->motion.xrel;
        factory->pos.y -= ev->motion.yrel;
        factory->panned = true;
        return false;
    }

    case SDL_MOUSEBUTTONDOWN: {
        SDL_MouseButtonEvent *b = &ev->button;
        if (b->button != SDL_BUTTON_LEFT) return false;
        factory->panning = true;
        return factory_event_click(factory);
    }

    case SDL_MOUSEBUTTONUP: {
        SDL_MouseButtonEvent *b = &ev->button;
        if (b->button != SDL_BUTTON_LEFT) return false;
        factory->panning = false;
        factory->panned = false;
        return false;
    }

    default: { return false; }
    }
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

static void factory_render_box(
        struct factory *factory, struct box *box, bool select, SDL_Renderer *renderer)
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

    ui_str_set_id(&factory->ui_id.str, box->id);
    ui_label_render(&factory->ui_id, &layout, renderer);
    ui_layout_next_row(&layout);

    ui_str_set_item(&factory->ui_target.str, box->target);
    ui_label_render(&factory->ui_target, &layout, renderer);
    if (box->loops != loops_inf) {
        ui_label_render(&factory->ui_loops, &layout, renderer);
        ui_str_set_u64(&factory->ui_loops_val.str, box->loops);
        ui_label_render(&factory->ui_loops_val, &layout, renderer);
    }
    ui_layout_next_row(&layout);

    if (box->in) {
        ui_str_set_item(&factory->ui_tape_in.str, box->in);
        ui_label_render(&factory->ui_tape_in, &layout, renderer);
    }
    else if (box->out) {
        ui_str_set_item(&factory->ui_tape_out.str, box->out);
        ui_label_render(&factory->ui_tape_out, &layout, renderer);
    }

    if ((box->in || box->out) && box->tape_len) {
        ui_str_set_u64(&factory->ui_tape_num.str, box->tape_it+1);
        ui_label_render(&factory->ui_tape_num, &layout, renderer);

        ui_label_render(&factory->ui_tape_of, &layout, renderer);

        ui_str_set_u64(&factory->ui_tape_num.str, box->tape_len);
        ui_label_render(&factory->ui_tape_num, &layout, renderer);
    }

    sdl_err(SDL_SetRenderTarget(renderer, NULL));
}

static void factory_render_op(
        struct factory *factory, uint64_t op, SDL_Renderer *renderer)
{
    struct htable_ret ret = {0};

    id_t src_id = 0, dst_id = 0;
    chunk_workers_ops(op, &src_id, &dst_id);

    ret = htable_get(&factory->index, src_id);
    struct box *src = (struct box *) ret.value;
    assert(ret.ok);

    ret = htable_get(&factory->index, dst_id);
    struct box *dst = (struct box *) ret.value;
    assert(ret.ok);

    struct pos src_pos = make_pos(
            src->col*factory->total.w + factory->out.w - factory->pos.x,
            src->row*factory->total.h + factory->out.h - factory->pos.y);

    struct pos dst_pos = make_pos(
            dst->col*factory->total.w + factory->in.w - factory->pos.x,
            dst->row*factory->total.h + factory->in.h - factory->pos.y);

    rgba_render(rgba_gray(0x88), renderer);
    sdl_err(SDL_RenderDrawLine(renderer, src_pos.x, src_pos.y, dst_pos.x, dst_pos.y));
}


void factory_render(struct factory *factory, SDL_Renderer *renderer)
{
    if (!factory->active) return;

    rgba_render(rgba_black(), renderer);
    sdl_err(SDL_RenderFillRect(renderer, &core.rect));

    size_t row = legion_max(factory->pos.y / factory->total.h, 0);
    size_t col = legion_max(factory->pos.x / factory->total.w, 0);

    size_t rows = i64_ceil_div(core.rect.h, factory->total.h);
    size_t cols = i64_ceil_div(core.rect.w, factory->total.w);

    struct box *cursor = factory_cursor_box(factory);

    for (size_t i = row; i < row + rows && i < vec64_len(factory->grid); ++i) {
        struct vec16 *vec = (void *) factory->grid->vals[i];

        for (size_t j = col; j < col + cols && j < vec16_len(vec); ++j) {
            struct box *box = &factory->boxes->vals[vec->vals[j]];
            factory_render_box(factory, box, box == cursor, renderer);

            SDL_Rect rect = {
                .x = j*factory->total.w - factory->pos.x,
                .y = i*factory->total.h - factory->pos.y,
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
