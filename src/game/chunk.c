/* chunk.c
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "chunk.h"

#include "game/world.h"
#include "game/sector.h"
#include "game/active.h"
#include "utils/vec.h"
#include "utils/ring.h"
#include "utils/htable.h"

static void chunk_ports_step(struct chunk *);

// -----------------------------------------------------------------------------
// cargo
// -----------------------------------------------------------------------------

struct chunk
{
    struct world *world;

    struct star star;
    active_list_t active;

    // ports
    struct htable provided;
    struct ring32 *requested;

    struct workers workers;
    struct ring64 *shuttles;
};

struct chunk *chunk_alloc(struct world *world, const struct star *star)
{
    struct chunk *chunk = calloc(1, sizeof(*chunk));
    chunk->world = world;
    chunk->star = *star;
    chunk->requested = ring32_reserve(16);
    chunk->shuttles = ring64_reserve(2);
    chunk->workers.ops = vec64_reserve(1);
    return chunk;
}

void chunk_free(struct chunk *chunk)
{
    active_it_t it = active_next(&chunk->active, NULL);
    for (; it; it = active_next(&chunk->active, it)) active_free(*it);

    for (struct htable_bucket *it = htable_next(&chunk->provided, NULL);
         it; it = htable_next(&chunk->provided, it))
    {
        ring32_free((void *) it->value);
    }
    htable_reset(&chunk->provided);
    ring32_free(chunk->requested);

    vec64_free(chunk->workers.ops);
    ring64_free(chunk->shuttles);

    free(chunk);
}

struct chunk *chunk_load(struct world *world, struct save *save)
{
    if (!save_read_magic(save, save_magic_chunk)) return NULL;

    struct chunk *chunk = calloc(1, sizeof(*chunk));
    chunk->world = world;
    star_load(&chunk->star, save);

    save_read_into(save, &chunk->workers.count);
    chunk->workers.ops = vec64_reserve(chunk->workers.count);
    chunk->shuttles = save_read_ring64(save);

    chunk->requested = save_read_ring32(save);
    size_t len = save_read_type(save, typeof(chunk->provided.len));

    htable_reserve(&chunk->provided, len);
    for (size_t i = 0; i < len; ++i) {
        enum item item = save_read_type(save, typeof(item));
        struct ring32 *ring = save_read_ring32(save);
        if (!ring) goto fail;

        struct htable_ret ret = htable_put(&chunk->provided, item, (uintptr_t) ring);
        assert(ret.ok);
    }

    active_list_load(&chunk->active, chunk, save);

    if (!save_read_magic(save, save_magic_chunk)) goto fail;
    return chunk;

  fail:
    chunk_free(chunk);
    return NULL;
}

void chunk_save(struct chunk *chunk, struct save *save)
{
    save_write_magic(save, save_magic_chunk);

    star_save(&chunk->star, save);
    save_write_value(save, chunk->workers.count);
    save_write_ring64(save, chunk->shuttles);

    save_write_ring32(save, chunk->requested);
    save_write_value(save, chunk->provided.len);
    struct htable_bucket *it = htable_next(&chunk->provided, NULL);
    for (; it; it = htable_next(&chunk->provided, it)) {
        save_write_value(save, (enum item) it->key);
        save_write_ring32(save, (struct ring32 *) it->value);
    }

    active_list_save(&chunk->active, save);

    save_write_magic(save, save_magic_chunk);
}

struct world *chunk_world(struct chunk *chunk)
{
    return chunk->world;
}

struct star *chunk_star(struct chunk *chunk)
{
    return &chunk->star;
}

bool chunk_harvest(struct chunk *chunk, enum item item)
{
    assert(item >= ITEM_NATURAL_FIRST && item < ITEM_SYNTH_FIRST);

    size_t i = item - ITEM_NATURAL_FIRST;
    if (!chunk->star.elems[i]) return false;
    chunk->star.elems[i]--;
    return true;
}

struct workers chunk_workers(struct chunk *chunk)
{
    return chunk->workers;
}

struct vec64* chunk_list(struct chunk *chunk)
{
    size_t sum = 0;
    active_it_t it = active_next(&chunk->active, NULL);
    for (; it; it = active_next(&chunk->active, it))
        sum += active_count(*it);

    struct vec64 *ids = vec64_reserve(sum);
    it = active_next(&chunk->active, NULL);
    for (; it; it = active_next(&chunk->active, it))
        active_list(*it, ids);

    return ids;
}

struct vec64* chunk_list_filter(struct chunk *chunk, im_list_t filter)
{
    size_t sum = 0;
    for (im_list_t it = filter; *it; it++)
        sum += active_count(active_index(&chunk->active, *it));

    struct vec64 *ids = vec64_reserve(sum);
    for (im_list_t it = filter; *it; it++)
        active_list(active_index(&chunk->active, *it), ids);

    return ids;
}

const void *chunk_get(struct chunk *chunk, id_t id)
{
    return active_get(active_index(&chunk->active, id_item(id)), id);
}

bool chunk_copy(struct chunk *chunk, id_t id, void *dst, size_t len)
{
    return active_copy(active_index(&chunk->active, id_item(id)), id, dst, len);
}

void chunk_create(struct chunk *chunk, enum item item)
{
    if (item == ITEM_WORKER) { chunk->workers.count++; return; }
    active_create(active_index_create(&chunk->active, item));
}

void chunk_create_from(
        struct chunk *chunk, enum item item, const word_t *data, size_t len)
{
    active_create_from(active_index_create(&chunk->active, item), chunk, data, len);
}

void chunk_delete(struct chunk *chunk, id_t id)
{
    active_delete(active_index(&chunk->active, id_item(id)), id);
}

void chunk_step(struct chunk *chunk)
{
    active_it_t it = active_next(&chunk->active, NULL);
    for (; it; it = active_next(&chunk->active, it))
        active_step(*it, chunk);

    chunk_ports_step(chunk);
}

bool chunk_io(
        struct chunk *chunk,
        enum io io, id_t src, id_t dst, const word_t *args, size_t len)
{
    if (!item_is_active(id_item(dst))) return false;
    struct active *active = active_index(&chunk->active, id_item(dst));
    return active_io(active, chunk, io, src, dst, len, args);
}


// -----------------------------------------------------------------------------
// scan
// -----------------------------------------------------------------------------

ssize_t chunk_scan(struct chunk *chunk, enum item item)
{
    if (item == ITEM_WORKER) return chunk->workers.count;
    if (item == ITEM_BULLET) return ring64_len(chunk->shuttles);

    if (item >= ITEM_NATURAL_FIRST && item < ITEM_NATURAL_LAST)
        return chunk->star.elems[item - ITEM_NATURAL_FIRST];

    if (item >= ITEM_ACTIVE_FIRST && item < ITEM_ACTIVE_LAST)
        return active_count(active_index(&chunk->active, item));

    return -1;
}

// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

void chunk_lanes_launch(
        struct chunk *chunk,
        enum item type,
        struct coord dst,
        const word_t *data, size_t len)
{
    world_lanes_launch(chunk->world, type, chunk->star.coord, dst, data, len);
}

void chunk_lanes_arrive(
        struct chunk *chunk, enum item item, const word_t *data, size_t len)
{
    switch (item)
    {
    case ITEM_ACTIVE_FIRST...ITEM_ACTIVE_LAST: {
        chunk_create_from(chunk, item, data, len);
        break;
    }
    case ITEM_BULLET: {
        assert(len == 1);
        assert(data[0] > 0 && data[0] <= UINT8_MAX);
        uint64_t value = (((uint64_t) item) << 8) | data[0];
        chunk->shuttles = ring64_push(chunk->shuttles, value);
        break;
    }
    default: { assert(false); }
    }
}

bool chunk_lanes_dock(struct chunk *chunk, enum item *item, uint8_t *count)
{
    if (ring64_empty(chunk->shuttles)) return false;

    uint64_t value = ring64_pop(chunk->shuttles);
    *item = value >> 8;
    *count = value & ((1ULL << 8) - 1);
    return true;
}


// -----------------------------------------------------------------------------
// ports
// -----------------------------------------------------------------------------

void chunk_ports_reset(struct chunk *chunk, id_t id)
{
    struct ports *ports = active_ports(active_index(&chunk->active, id_item(id)), id);
    if (!ports) return;

    if (ports->in_state == ports_requested)
        ring32_replace(chunk->requested, id, 0);

    if (ports->out) {
        struct htable_ret ret = htable_get(&chunk->provided, ports->out);
        assert(ret.ok);

        ring32_replace((struct ring32 *) ret.value, id, 0);
    }

    *ports = (struct ports) {0};
}

bool chunk_ports_produce(struct chunk *chunk, id_t id, enum item item)
{
    struct ports *ports = active_ports(active_index(&chunk->active, id_item(id)), id);
    if (!ports) return false;

    if (ports->out != ITEM_NIL) return false;
    ports->out = item;

    struct ring32 *provided = NULL;
    struct htable_ret hret = htable_get(&chunk->provided, item);

    if (likely(hret.ok)) provided = (struct ring32 *) hret.value;
    else {
        provided = ring32_reserve(1);
        hret = htable_put(&chunk->provided, item, (uint64_t) provided);
        assert(hret.ok);
    }
    assert(provided);

    struct ring32 *rret = ring32_push(provided, id);
    if (unlikely(rret != provided)) {
        hret = htable_xchg(&chunk->provided, item, (uint64_t) rret);
        assert(hret.ok);
    }

    return true;
}

bool chunk_ports_consumed(struct chunk *chunk, id_t id)
{
    struct ports *ports = active_ports(active_index(&chunk->active, id_item(id)), id);
    return ports && ports->out == ITEM_NIL;
}

void chunk_ports_request(struct chunk *chunk, id_t id, enum item item)
{
    struct ports *ports = active_ports(active_index(&chunk->active, id_item(id)), id);
    if (!ports) return;

    assert(ports->in_state == ports_nil);
    ports->in = item;
    ports->in_state = ports_requested;

    chunk->requested = ring32_push(chunk->requested, id);
}

enum item chunk_ports_consume(struct chunk *chunk, id_t id)
{
    struct ports *ports = active_ports(active_index(&chunk->active, id_item(id)), id);
    if (!ports) return ITEM_NIL;

    if (ports->in_state != ports_received) return ITEM_NIL;
    enum item ret = ports->in;
    ports->in = ITEM_NIL;
    ports->in_state = ports_nil;
    return ret;
}

static void chunk_ports_step(struct chunk *chunk)
{
    id_t fail = 0;
    chunk->workers.idle = 0;
    chunk->workers.fail = 0;
    chunk->workers.clean = 0;
    chunk->workers.queue = ring32_len(chunk->requested);
    chunk->workers.ops->len = 0;

    for (size_t i = 0; i < chunk->workers.count; ++i) {

        if (ring32_empty(chunk->requested) ||
                (fail && fail == ring32_peek(chunk->requested)))
        {
            chunk->workers.idle = chunk->workers.count - i;
            break;
        }

        id_t dst = ring32_pop(chunk->requested);
        if (!dst)  { chunk->workers.clean++; continue; }

        struct ports *in = active_ports(active_index(&chunk->active, id_item(dst)), dst);
        assert(in && in->in_state == ports_requested);

        struct htable_ret hret = htable_get(&chunk->provided, in->in);
        if (!hret.ok) goto nomatch;

        struct ring32 *provided = (struct ring32 *) hret.value;
        assert(provided);
        if (ring32_empty(provided)) goto nomatch;

        id_t src = ring32_pop(provided);
        if (!src) { chunk->workers.clean++; goto nomatch; }

        struct ports *out = active_ports(active_index(&chunk->active, id_item(src)), src);
        assert(out && out->out == in->in);

        out->out = ITEM_NIL;
        in->in_state = ports_received;

        chunk->workers.ops =
            vec64_append(chunk->workers.ops, ((uint64_t) src << 32) | dst);

        continue;

      nomatch:
        struct ring32 *rret = ring32_push(chunk->requested, dst);
        assert(rret == chunk->requested);
        if (!fail) fail = dst;
        chunk->workers.fail++;
        continue;
    }
}
