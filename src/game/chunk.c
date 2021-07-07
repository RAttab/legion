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
// chunk_cargo
// -----------------------------------------------------------------------------

struct legion_packed chunk_cargo
{
    enum item type;
    enum item cargo;
    uint8_t count;
    legion_pad(1);
};
static_assert(sizeof(struct chunk_cargo) == 4);

inline uint32_t chunk_cargo_to_u32(struct chunk_cargo val)
{
    return (union { struct chunk_cargo from; uint32_t to; }) {.from = val}.to;
}


inline struct chunk_cargo chunk_cargo_from_u32(uint32_t val)
{
    return (union { struct chunk_cargo to; uint32_t from; }) {.from = val}.to;
}

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
    struct ring32 *arrivals;
};

struct chunk *chunk_alloc(struct world *world, const struct star *star)
{
    struct chunk *chunk = calloc(1, sizeof(*chunk));
    chunk->world = world;
    chunk->star = *star;
    chunk->requested = ring32_reserve(16);
    htable_reset(&chunk->provided);
    return chunk;
}

void chunk_free(struct chunk *chunk)
{
    active_it_t it = active_next(&chunk->active, NULL);
    for (; it; it = active_next(&chunk->active, it)) active_free(*it);

    free(chunk->requested);
    htable_reset(&chunk->provided);
    free(chunk);
}

struct chunk *chunk_load(struct world *world, struct save *save)
{
    if (!save_read_magic(save, save_magic_chunk)) return NULL;

    struct chunk *chunk = calloc(1, sizeof(*chunk));
    chunk->world = world;
    star_load(&chunk->star, save);

    save_read_into(save, &chunk->workers.count);
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
    if (!chunk->star.elems[item]) return false;

    chunk->star.elems[item]--;
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

struct vec64* chunk_list_filter(
        struct chunk *chunk, const enum item *filter, size_t len)
{
    size_t sum = 0;
    for (size_t i = 0; i < len; ++i)
        sum += active_count(active_index(&chunk->active, filter[i]));

    struct vec64 *ids = vec64_reserve(sum);
    for (size_t i = 0; i < len; ++i)
        active_list(active_index(&chunk->active, filter[i]), ids);

    return ids;
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
        enum atom_io io, id_t src, id_t dst, size_t len, const word_t *args)
{
    struct active *active = active_index(&chunk->active, id_item(dst));
    return active_io(active, chunk, io, src, dst, len, args);
}

ssize_t chunk_scan(struct chunk *chunk, enum item item)
{
    switch (item) {

    case ITEM_WORKER: { return chunk->workers.count; }

    case ITEM_NATURAL_FIRST...ITEM_SYNTH_FIRST: {
        return chunk->star.elems[item - ITEM_NATURAL_FIRST];
    }

    case ITEM_ACTIVE_FIRST...ITEM_ACTIVE_LAST: {
        return active_count(active_index(&chunk->active, item));
    }

    default: { return -1; }

    }
}

void chunk_lanes_arrive(struct chunk *chunk, enum item type, enum item cargo, uint8_t count)
{
    struct chunk_cargo arrival = {.type = type, .cargo = cargo, .count = count };
    chunk->arrivals = ring32_push(chunk->arrivals, chunk_cargo_to_u32(arrival));
}


// -----------------------------------------------------------------------------
// ports
// -----------------------------------------------------------------------------

void chunk_ports_reset(struct chunk *chunk, id_t id)
{
    struct ports *ports = active_ports(active_index(&chunk->active, id_item(id)), id);
    if (likely(ports)) ports_reset(ports);
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

void chunk_ports_request(struct chunk *chunk, id_t id, enum item item)
{
    struct ports *ports = class_ports(chunk_class(chunk, id_item(id)), id);
    if (!ports) return;

    assert(ports->in_state == ports_nil);
    ports->in = item;
    ports->in_state = ports_requested;

    ring32_push(chunk->requested, id);
}

enum item chunk_ports_consume(struct chunk *chunk, id_t id)
{
    struct ports *ports = class_ports(chunk_class(chunk, id_item(id)), id);
    if (!ports) return ITEM_NIL;

    if (ports->in_state != ports_received) return ITEM_NIL;
    enum item ret = ports->in;
    ports->in = ITEM_NIL;
    ports->in_state = ports_nil;
    return ret;
}

static void chunk_ports_step(struct chunk *chunk)
{
    chunk->workers.idle = 0;
    chunk->workers.fail = 0;

    for (size_t i = 0; i < chunk->workers.count; ++i) {
        id_t dst = ring32_pop(chunk->requested);
        if (!dst) { chunk->workers.idle = chunk->workers.count - i; break; }

        struct ports *in = class_ports(chunk_class(chunk, id_item(dst)), dst);
        assert(in);
        if (in->in_state != ports_requested) goto nomatch;

        struct htable_ret hret = htable_get(&chunk->provided, in->in);
        if (!hret.ok) goto nomatch;

        struct ring32 *provided = (struct ring32 *) hret.value;
        assert(provided);
        if (ring32_empty(provided)) goto nomatch;

        id_t src = ring32_pop(provided);
        struct ports *out = class_ports(chunk_class(chunk, id_item(src)), src);
        assert(out);

        in->in_state = ports_received;
        out->out = ITEM_NIL;
        continue;

      nomatch:
       struct ring32 *rret = ring32_push(chunk->requested, dst);
       assert(rret == chunk->requested);
       chunk->workers.fail++;
    }
}
