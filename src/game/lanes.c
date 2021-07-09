/* lanes.c
   Rémi Attab (remi.attab@gmail.com), 03 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/lanes.h"
#include "game/item.h"
#include "utils/vec.h"


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

legion_packed struct lane_heap
{
    world_ts_t ts;
    uint16_t data;
    legion_pad(2);
};

static_assert(sizeof(struct lane_heap) == 8);


legion_packed struct lane_data
{
    uint32_t data;
    uint16_t next;
    enum item item;
    bool direction;
};

static_assert(sizeof(struct lane_data) == 8);


struct lane
{
    struct coord src, dst;

    uint16_t free, len, cap;
    struct lane_heap *heap;
    struct lane_data *data;
};

static struct lane *lane_alloc(struct coord src, struct coord dst)
{
    enum { cap_default = 1 };

    struct lane *lane = calloc(1, sizeof(*lane));
    *lane = (struct lane) {
        .src = src,
        .dst = dst,
        .cap = cap_default,
        .free = 0,
        .heap = calloc(cap_default, sizeof(struct lane_heap)),
        .data = calloc(cap_default, sizeof(struct lane_data)),
    };
    return lane;
}

static void lane_free(struct lane *lane)
{
    free(lane->heap);
    free(lane->data);
    free(lane);
}

static struct lane *lane_load(struct save *save)
{
    if (!save_read_magic(save, save_magic_lane)) return NULL;

    struct lane *lane = calloc(1, sizeof(*lane));
    save_read_into(save, &lane->src);
    save_read_into(save, &lane->dst);
    save_read_into(save, &lane->free);
    save_read_into(save, &lane->len);
    save_read_into(save, &lane->cap);

    lane->heap = calloc(lane->cap, sizeof(*lane->heap));
    save_read(save, lane->heap, lane->cap * sizeof(*lane->heap));

    lane->data = calloc(lane->cap, sizeof(*lane->data));
    save_read(save, lane->data, lane->cap * sizeof(*lane->data));

    if (!save_read_magic(save, save_magic_lane)) goto fail;
    return lane;

  fail:
    free(lane->heap);
    free(lane->data);
    free(lane);
    return NULL;
}

static void lane_save(struct lane *lane, struct save *save)
{
    save_write_magic(save, save_magic_lane);
    save_write_value(save, lane->src);
    save_write_value(save, lane->dst);
    save_write_value(save, lane->free);
    save_write_value(save, lane->len);
    save_write_value(save, lane->cap);
    save_write(save, lane->heap, lane->cap * sizeof(*lane->heap));
    save_write(save, lane->data, lane->cap * sizeof(*lane->data));
    save_write_magic(save, save_magic_lane);
}

static void lane_grow(struct lane *lane)
{
    if (likely(lane->len != lane->cap)) return;

    lane->cap *= 2;
    lane->heap = reallocarray(lane->heap, lane->cap, sizeof(*lane->heap));
    lane->data = reallocarray(lane->data, lane->cap, sizeof(*lane->data));

    lane->free = lane->len;
    for (size_t i = lane->len; i < lane->cap; ++i)
        lane->data[i].next = i+1;
}

static void lane_push(struct lane *lane,
        world_ts_t ts, struct coord src, enum item type, uint32_t data)
{
    lane_grow(lane);

    uint16_t index = lane->free;
    struct lane_data *entry = lane->data + index;
    lane->free = entry->next;
    *entry = (struct lane_data) {
        .data = data,
        .item = type,
        .direction = coord_eq(src, lane->src),
    };

    struct lane_heap *it = &lane->heap[lane->len++];
    *it = (struct lane_heap) { .ts = ts, .data = index };

    while (index) {
        struct lane_heap *parent = lane->heap + (index / 2);
        if (parent->ts < it->ts) break;

        legion_swap(it, parent);
        it = parent;
        index /= 2;
    }
}

static world_ts_t lane_peek(struct lane *lane)
{
    return lane->len ? lane->heap[0].ts : (world_ts_t) -1;
}

static struct lane_data lane_pop(struct lane *lane)
{
    assert(lane->len);

    size_t ret = lane->heap[0].data;
    lane->heap[0] = lane->heap[--lane->len];

    size_t index = 0;
    while (index < lane->len) {
        struct lane_heap *it = lane->heap + index;

        size_t li = index * 2;
        size_t ri = index * 2 + 1;

        struct lane_heap *lp = li < lane->len ? lane->heap + li : NULL;
        struct lane_heap *rp = ri < lane->len ? lane->heap + ri : NULL;

        size_t child = 0;
        struct lane_heap *ptr = NULL;

        if ((lp && lp->ts < it->ts) && (!rp || lp->ts < rp->ts)) {
            child = li;
            ptr = lp;
        }
        else if (rp && rp->ts < it->ts) {
            child = ri;
            ptr = rp;
        }
        else break;

        legion_swap(it, ptr);
        index = child;
        it = ptr;
    }

    lane->data[ret].next = lane->free;
    lane->free = ret;
    return lane->data[ret];
}


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

void lanes_init(struct lanes *lanes, struct world *world)
{
    lanes->world = world;
}

void lanes_free(struct lanes *lanes)
{
    struct htable_bucket *it = NULL;

    it = htable_next(&lanes->lanes, NULL);
    for (; it; it = htable_next(&lanes->lanes, it))
        lane_free((void *) it->value);
    htable_reset(&lanes->lanes);

    it = htable_next(&lanes->index, NULL);
    for (; it; it = htable_next(&lanes->index, it))
        vec64_free((void *) it->value);
    htable_reset(&lanes->index);
}

static uint64_t lanes_key(struct coord src, struct coord dst)
{
    return coord_to_id(src) ^ coord_to_id(dst);
}

static void lanes_index(struct lanes *lanes, struct coord key, struct coord val)
{
    uint64_t key64 = coord_to_id(key);
    struct htable_ret ret = htable_get(&lanes->index, key64);

    struct vec64 *old = (void *) ret.value;
    struct vec64 *new = vec64_append(old, coord_to_id(val));
    if (old == new) return;

    if (old)
        ret = htable_xchg(&lanes->index, key64, (uintptr_t) new);
    else ret = htable_put(&lanes->index, key64, (uintptr_t) new);
    assert(ret.ok);
}

bool lanes_load(struct lanes *lanes, struct world *world, struct save *save)
{
    if (!save_read_magic(save, save_magic_lanes)) return false;
    lanes->world = world;

    uint32_t len = save_read_type(save, typeof(len));
    for (size_t i = 0; i < len; ++i) {
        struct lane *lane = lane_load(save);
        if (!lane) goto fail;

        uint64_t key = lanes_key(lane->src, lane->dst);
        struct htable_ret ret = htable_put(&lanes->lanes, key, (uintptr_t) lane);
        assert(ret.ok);

        lanes_index(lanes, lane->src, lane->dst);
        lanes_index(lanes, lane->dst, lane->src);
    }

    if (!save_read_magic(save, save_magic_lanes)) goto fail;
    return true;

  fail:
    lanes_free(lanes);
    return false;
}

void lanes_save(struct lanes *lanes, struct save *save)
{
    save_write_magic(save, save_magic_lanes);
    save_write_value(save, lanes->lanes.len);

    struct htable_bucket *it = htable_next(&lanes->lanes, NULL);
    for (; it; it = htable_next(&lanes->lanes, it))
        lane_save((void *) it->value, save);

    save_write_magic(save, save_magic_lanes);
}

struct vec64 *lanes_list(struct lanes *lanes, struct coord key)
{
    struct htable_ret ret = htable_get(&lanes->index, coord_to_id(key));
    return (void *) ret.value;
}

static uint64_t lanes_item_div(enum item type)
{
    switch (type) {

    case ITEM_LEGION_1:
    case ITEM_SHUTTLE_1: { return 1; }

    case ITEM_LEGION_2:
    case ITEM_SHUTTLE_2: { return 10; }

    case ITEM_LEGION_3:
    case ITEM_SHUTTLE_3: { return 100; }

    default: { assert(false); }
    };
}

void lanes_launch(struct lanes *lanes,
        struct coord src, struct coord dst, enum item type, uint32_t data)
{
    uint64_t key = lanes_key(src, dst);
    struct htable_ret ret = htable_get(&lanes->lanes, key);

    struct lane *lane = NULL;
    if (ret.ok) lane = (void *) ret.value;
    else {
        lane = lane_alloc(src, dst);
        ret = htable_put(&lanes->lanes, key, (uintptr_t) lane);
        assert(ret.ok);

        lanes_index(lanes, src, dst);
        lanes_index(lanes, dst, src);
    }

    world_ts_t now = world_time(lanes->world);
    world_ts_t ts = now + (coord_dist(src, dst) / lanes_item_div(type));
    lane_push(lane, ts, src, type, data);
}

void lanes_step(struct lanes *lanes)
{
    world_ts_t now = world_time(lanes->world);

    struct htable_bucket *it = htable_next(&lanes->lanes, NULL);
    for (; it; it = htable_next(&lanes->lanes, it)) {
        struct lane *lane = (void *) it->value;

        while (lane_peek(lane) < now) {
            struct lane_data data = lane_pop(lane);
            struct coord dst = data.direction ? lane->dst : lane->src;
            world_lanes_arrive(lanes->world, dst, data.item, data.data);
        }
    }
}
