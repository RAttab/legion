/* lanes.c
   Rémi Attab (remi.attab@gmail.com), 03 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/lanes.h"


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

enum { lanes_min_cap = 32 };

void lanes_init(struct lanes *lanes, struct world *world)
{
    *lanes = (struct lanes) {
        .world = world,
        .len = 0,
        .cap = lanes_min_cap,
        .data = calloc(lanes_min_cap, sizeof(struct cargo)),
    };
}

void lanes_free(struct lanes *lanes)
{
    free(lanes->data);
}


bool lanes_load(struct lanes *lanes, struct world *world, struct save *save)
{
    if (!save_read_magic(save, save_magic_lanes)) return false;

    lanes->world = world;
    lanes->len = save_read_type(save, uint32_t);

    lanes->cap = lanes_min_cap;
    while (lanes->cap < lanes->len) lanes->cap *= 2;
    lanes->data = calloc(lanes->cap, sizeof(lanes->data[0]));

    save_read(save, lanes->data, lanes->len * sizeof(lanes->data[0]));

    if (!save_read_magic(save, save_magic_lanes)) goto fail;
    return true;

  fail:
    free(lanes->data);
    return false;
}

void lanes_save(struct lanes *lanes, struct save *save)
{
    save_write_magic(save, save_magic_lanes);
    save_write_value(save, (uint32_t) lanes->len);
    save_write(save, lanes->data, lanes->len * sizeof(lanes->data[0]));
    save_write_magic(save, save_magic_lanes);
}

static world_ts_t lanes_arrival(
        struct lanes *lanes, struct coord src, struct coord dst, enum item type)
{
    world_ts_t now = world_time(lanes->world);
    uint64_t dist = coord_dist(src, dst);

    uint64_t div = 0;
    switch (type) {
    case ITEM_SHUTTLE_S: { div = 1; break; }
    case ITEM_SHUTTLE_M: { div = 10; break; }
    case ITEM_SHUTTLE_F: { div = 100; break; }
    default: { assert(false); }
    };

    return now + (dist / div);
}


void lanes_launch(
        struct lanes *lanes,
        struct coord src, struct coord dst,
        enum item type, enum item cargo, uint8_t count)
{
    size_t index = lanes->len++;
    struct cargo *it = &lanes->data[index];

    *it = (struct cargo) {
        .ts = lanes_arrival(lanes, src, dst, type),
        .type = type, .cargo = cargo, .count = count, .dst = dst,
    };

    while (index) {
        struct cargo *parent = &lanes->data[index / 2];
        if (parent->ts < it->ts) break;

        legion_swap(it, parent);
        it = parent;
        index /= 2;
    }
}

static void lanes_pop(struct lanes *lanes)
{
    lanes->data[0] = lanes->data[--lanes->len];

    size_t index = 0;
    while (index < lanes->len) {
        struct cargo *it = &lanes->data[index];

        size_t li = index * 2;
        size_t ri = index * 2 + 1;

        struct cargo *lp = li < lanes->len ? &lanes->data[li] : NULL;
        struct cargo *rp = ri < lanes->len ? &lanes->data[ri] : NULL;

        size_t index = 0;
        struct cargo *ptr = NULL;

        if ((lp && lp->ts < it->ts) && (!rp || lp->ts < rp->ts)) {
            index = li;
            ptr = lp;
        }
        else if (rp && rp->ts < it->ts) {
            index = ri;
            ptr = rp;
        }
        else break;

        legion_swap(it, ptr);
        index = index;
        it = ptr;
    }
}

void lanes_step(struct lanes *lanes)
{
    world_ts_t now = world_time(lanes->world);
    while (lanes->len && lanes->data[0].ts <= now) {
        struct cargo *it = &lanes->data[0];
        world_lanes_arrive(lanes->world, it->dst, it->type, it->cargo, it->count);
        lanes_pop(lanes);
    }
}
