/* chunk.c
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "chunk.h"

#include "game/log.h"
#include "game/world.h"
#include "game/sector.h"
#include "game/active.h"
#include "game/energy.h"
#include "items/types.h"
#include "utils/vec.h"
#include "utils/ring.h"
#include "utils/htable.h"

static void chunk_ports_step(struct chunk *);

// -----------------------------------------------------------------------------
// cargo
// -----------------------------------------------------------------------------

enum { chunk_log_cap = 8 };

struct chunk
{
    struct world *world;
    struct star star;
    word_t name;

    struct log *log;

    // Ports
    struct htable provided;
    struct ring32 *requested;
    struct ring32 *storage;

    // Logistics
    struct energy energy;
    struct workers workers;
    struct ring64 *pills;

    struct htable listen;

    struct active active[ITEMS_ACTIVE_LEN];
};

static struct active *active_index(struct chunk *chunk, enum item item)
{
    size_t index = item - ITEM_ACTIVE_FIRST;

    if (!item_is_active(item)) return NULL;
    if (index >= array_len(chunk->active)) return NULL;
    if (chunk->active[index].skip) return NULL;

    return chunk->active + index;
}

static struct active *active_index_assert(struct chunk *chunk, enum item item)
{
    struct active *it = active_index(chunk, item);
    assert(it);
    return it;
}

static struct active *active_next(struct chunk *chunk, struct active *it)
{
    it = it ? it + 1 : chunk->active;
    for (; it < chunk->active + array_len(chunk->active); it++) {
        if (it->skip) continue;
        return it;
    }
    return NULL;
}

struct chunk *chunk_alloc(
        struct world *world, const struct star *star, word_t name)
{
    assert(world);

    struct chunk *chunk = calloc(1, sizeof(*chunk));
    *chunk = (struct chunk) {
        .world = world,
        .star = *star,
        .name = name,
        .log = log_new(chunk_log_cap),
        .requested = ring32_reserve(16),
        .storage = ring32_reserve(1),
        .pills = ring64_reserve(2),
        .workers.ops = vec64_reserve(1),
    };

    for (size_t i = 0; i < array_len(chunk->active); ++i)
        active_init(chunk->active + i, ITEM_ACTIVE_FIRST + i);

    return chunk;
}

void chunk_free(struct chunk *chunk)
{
    log_free(chunk->log);

    ring32_free(chunk->requested);
    ring32_free(chunk->storage);

    for (const struct htable_bucket *it = htable_next(&chunk->provided, NULL);
         it; it = htable_next(&chunk->provided, it))
    {
        ring32_free((void *) it->value);
    }
    htable_reset(&chunk->provided);

    ring64_free(chunk->pills);
    vec64_free(chunk->workers.ops);

    for (const struct htable_bucket *it = htable_next(&chunk->listen, NULL);
         it; it = htable_next(&chunk->listen, it))
    {
        free((void *) it->value);
    }
    htable_reset(&chunk->listen);

    for (struct active *it = active_next(chunk, NULL); it; it = active_next(chunk, it))
        active_free(it);

    free(chunk);
}

void chunk_save(struct chunk *chunk, struct save *save)
{
    save_write_magic(save, save_magic_chunk);

    save_write_value(save, chunk->name);
    star_save(&chunk->star, save);
    log_save(chunk->log, save);

    save_write_ring32(save, chunk->requested);
    save_write_ring32(save, chunk->storage);
    save_write_value(save, chunk->provided.len);
    for (const struct htable_bucket *it = htable_next(&chunk->provided, NULL);
         it; it = htable_next(&chunk->provided, it))
    {
        save_write_value(save, (enum item) it->key);
        save_write_ring32(save, (struct ring32 *) it->value);
    }

    energy_save(&chunk->energy, save);
    save_write_value(save, chunk->workers.count);
    save_write_vec64(save, chunk->workers.ops);
    save_write_ring64(save, chunk->pills);

    save_write_value(save, chunk->listen.len);
    for (const struct htable_bucket *it = htable_next(&chunk->listen, NULL);
         it; it = htable_next(&chunk->listen, it))
    {
        save_write_value(save, it->key);
        save_write(save, (id_t *) it->value, im_channel_max * sizeof(id_t));
    }

    for (struct active *it = active_next(chunk, NULL); it; it = active_next(chunk, it))
        active_save(it, save);

    save_write_magic(save, save_magic_chunk);
}

struct chunk *chunk_load(struct world *world, struct save *save)
{
    if (!save_read_magic(save, save_magic_chunk)) return NULL;

    struct chunk *chunk = calloc(1, sizeof(*chunk));
    chunk->world = world;

    save_read_into(save, &chunk->name);
    star_load(&chunk->star, save);

    if (!(chunk->log = log_load(save))) goto fail;

    chunk->requested = save_read_ring32(save);
    chunk->storage = save_read_ring32(save);
    { // provided
        size_t len = save_read_type(save, typeof(chunk->provided.len));
        htable_reserve(&chunk->provided, len);
        for (size_t i = 0; i < len; ++i) {
            enum item item = save_read_type(save, typeof(item));

            struct ring32 *ring = save_read_ring32(save);
            if (!ring) goto fail;

            struct htable_ret ret = htable_put(&chunk->provided, item, (uintptr_t) ring);
            assert(ret.ok);
        }
    }

    if (!energy_load(&chunk->energy, save)) goto fail;
    save_read_into(save, &chunk->workers.count);
    if (!save_read_vec64(save, &chunk->workers.ops)) goto fail;
    chunk->pills = save_read_ring64(save);

    { // listen
        size_t len = save_read_type(save, typeof(chunk->listen.len));
        htable_reserve(&chunk->listen, len);
        for (size_t i = 0; i < len; ++i) {
            uint64_t src = save_read_type(save, typeof(src));

            id_t *channels = calloc(im_channel_max, sizeof(*channels));
            save_read(save, channels, im_channel_max * sizeof(*channels));
            struct htable_ret ret = htable_put(&chunk->listen, src, (uintptr_t) channels);
            assert(ret.ok);
        }
    }

    for (size_t i = 0; i < array_len(chunk->active); ++i) {
        struct active *it = chunk->active + i;

        active_init(it, ITEM_ACTIVE_FIRST + i);
        if (it->skip) continue;

        if (!active_load(it, save, chunk->world ? chunk : NULL)) goto fail;
    }

    if (!save_read_magic(save, save_magic_chunk)) goto fail;
    return chunk;

  fail:
    chunk_free(chunk);
    return NULL;
}

struct world *chunk_world(struct chunk *chunk)
{
    assert(chunk->world);
    return chunk->world;
}

const struct star *chunk_star(const struct chunk *chunk)
{
    return &chunk->star;
}

word_t chunk_name(struct chunk *chunk)
{
    return chunk->name;
}
void chunk_rename(struct chunk *chunk, word_t new)
{
    chunk->name = new;
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
    for (struct active *it = active_next(chunk, NULL); it; it = active_next(chunk, it))
        sum += active_count(it);

    struct vec64 *ids = vec64_reserve(sum);
    for (struct active *it = active_next(chunk, NULL); it; it = active_next(chunk, it))
        active_list(it, ids);

    return ids;
}

struct vec64* chunk_list_filter(struct chunk *chunk, im_list_t filter)
{
    size_t sum = 0;
    for (im_list_t it = filter; *it; it++)
        sum += active_count(active_index_assert(chunk, *it));

    struct vec64 *ids = vec64_reserve(sum);
    for (im_list_t it = filter; *it; it++)
        active_list(active_index_assert(chunk, *it), ids);

    return ids;
}

const void *chunk_get(struct chunk *chunk, id_t id)
{
    return active_get(active_index_assert(chunk, id_item(id)), id);
}

bool chunk_copy(struct chunk *chunk, id_t id, void *dst, size_t len)
{
    return active_copy(active_index_assert(chunk, id_item(id)), id, dst, len);
}

static bool chunk_create_logistics(struct chunk *chunk, enum item item)
{
    if (likely(!item_is_logistics(item))) return false;

    switch (item) {
    case ITEM_WORKER:  { chunk->workers.count++; return true; }
    case ITEM_SOLAR:   { chunk->energy.solar++; return true; }
    case ITEM_KWHEEL:  { chunk->energy.kwheel++; return true; }
    case ITEM_BATTERY: { chunk->energy.battery++; return true; }

    case ITEM_PILL: {
        word_t cargo = 0;
        chunk_lanes_arrive(chunk, item, coord_nil(), &cargo, 1);
        return true;
    }

    default: { assert(false); }
    }
}

void chunk_create(struct chunk *chunk, enum item item)
{
    if (chunk_create_logistics(chunk, item)) return;
    active_create(active_index_assert(chunk, item));
}

void chunk_create_from(
        struct chunk *chunk, enum item item, const word_t *data, size_t len)
{

    if (chunk_create_logistics(chunk, item)) { assert(!len); return; }
    active_create_from(active_index_assert(chunk, item), chunk, data, len);
}

void chunk_delete(struct chunk *chunk, id_t id)
{
    active_delete(active_index_assert(chunk, id_item(id)), id);
}

void chunk_step(struct chunk *chunk)
{
    energy_step_begin(&chunk->energy, &chunk->star);

    for (struct active *it = active_next(chunk, NULL); it; it = active_next(chunk, it))
        active_step(it, chunk);
    chunk_ports_step(chunk);

    energy_step_end(&chunk->energy);
}

bool chunk_io(
        struct chunk *chunk,
        enum io io, id_t src, id_t dst, const word_t *args, size_t len)
{
    struct active *active = active_index(chunk, id_item(dst));
    if (!active) return false;

    return active_io(active, chunk, io, src, dst, args, len);
}


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

void chunk_log(struct chunk *chunk, id_t id, enum io io, enum ioe err)
{
    assert(chunk->world);

    struct coord star = chunk->star.coord;
    log_push(chunk->log, world_time(chunk->world), star, id, io, err);
    world_log_push(chunk->world, star, id, io, err);
}

const struct log *chunk_logs(struct chunk *chunk)
{
    return chunk->log;
}


// -----------------------------------------------------------------------------
// energy
// -----------------------------------------------------------------------------

struct energy *chunk_energy(struct chunk *chunk)
{
    return &chunk->energy;
}


// -----------------------------------------------------------------------------
// scan
// -----------------------------------------------------------------------------

ssize_t chunk_scan(struct chunk *chunk, enum item item)
{
    switch (item)
    {

    case ITEM_WORKER:  { return chunk->workers.count; }
    case ITEM_PILL:    { return ring64_len(chunk->pills); }
    case ITEM_SOLAR:   { return chunk->energy.solar; }
    case ITEM_KWHEEL:  { return chunk->energy.kwheel; }
    case ITEM_BATTERY: { return chunk->energy.battery; }
    case ITEM_ENERGY:  { return chunk->star.energy; }

    case ITEM_NATURAL_FIRST ... ITEM_NATURAL_LAST: {
        if (item == ITEM_NATURAL_LAST) return -1;
        return chunk->star.elems[item - ITEM_NATURAL_FIRST];
    }
    case ITEM_ACTIVE_FIRST ... ITEM_ACTIVE_LAST: {
        struct active *active = active_index(chunk, item);
        return active ? (ssize_t) active_count(active) : (ssize_t) -1;
    }

    default: { return -1; }

    }
}

// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

void chunk_lanes_listen(
        struct chunk *chunk, id_t id, struct coord src, uint8_t chan)
{
    assert(chan < im_channel_max);

    const uint64_t key = coord_to_u64(src);
    struct htable_ret ret = htable_get(&chunk->listen, key);

    id_t *channels = (void *) ret.value;
    if (!ret.ok) {
        channels = calloc(im_channel_max, sizeof(*channels));
        ret = htable_put(&chunk->listen, key, (uintptr_t) channels);
        assert(ret.ok);
    }

    if (!channels[chan]) channels[chan] = id;
}

void chunk_lanes_unlisten(
        struct chunk *chunk, id_t id, struct coord src, uint8_t chan)
{
    assert(chan < im_channel_max);

    const uint64_t key = coord_to_u64(src);
    struct htable_ret ret = htable_get(&chunk->listen, key);
    if (!ret.ok) return;

    id_t *channels = (void *) ret.value;
    if (channels[chan] != id) return;

    channels[chan] = 0;

    for (size_t i = 0; i < im_channel_max; ++i)
        if (channels[i]) return;
    free(channels);
    ret = htable_del(&chunk->listen, key);
    assert(ret.ok);
}

static void chunk_lanes_receive(
        struct chunk *chunk, struct coord src, const word_t *data, size_t len)
{
    assert(len >= 1);

    uint8_t packet_chan = 0, packet_len = 0;
    im_packet_unpack(data[0], &packet_chan, &packet_len);

    struct htable_ret ret = htable_get(&chunk->listen, coord_to_u64(src));
    if (!ret.ok) return;

    id_t dst = ((id_t *) ret.value)[packet_chan];
    if (!dst) return;

    chunk_io(chunk, IO_RECV, 0, dst, data, len);
}

void chunk_lanes_arrive(
        struct chunk *chunk,
        enum item item, struct coord src,
        const word_t *data, size_t len)
{
    switch (item)
    {

    case ITEM_ACTIVE_FIRST...ITEM_ACTIVE_LAST: {
        chunk_create_from(chunk, item, data, len);
        break;
    }

    case ITEM_PILL: {
        word_t cargo = len == 1 ? data[0] : 0;
        chunk->pills = ring64_push(chunk->pills, cargo);
        break;
    }

    case ITEM_DATA: {
        chunk_lanes_receive(chunk, src, data, len);
        break;
    }

    default: { assert(false); }
    }
}

bool chunk_lanes_dock(struct chunk *chunk, word_t *data)
{
    if (ring64_empty(chunk->pills)) return false;
    *data = ring64_pop(chunk->pills);
    return true;
}

void chunk_lanes_launch(
        struct chunk *chunk,
        enum item type, size_t speed,
        struct coord dst,
        const word_t *data, size_t len)
{
    assert(chunk->world);

    switch (type)
    {
    case ITEM_ACTIVE_FIRST...ITEM_ACTIVE_LAST: { break; }
    case ITEM_PILL: { break; }
    case ITEM_DATA: { break; }
    default: { assert(false); }
    }

    world_lanes_launch(chunk->world, type, speed, chunk->star.coord, dst, data, len);
}


// -----------------------------------------------------------------------------
// ports
// -----------------------------------------------------------------------------

static void ring32_replace(struct ring32 *ring, uint32_t old, uint32_t new)
{
    for (size_t i = 0; i < ring->cap; ++i) {
        if (ring->vals[i] == old) ring->vals[i] = new;
    }
}

void chunk_ports_reset(struct chunk *chunk, id_t id)
{
    struct active *active = active_index_assert(chunk, id_item(id));
    struct ports *ports = active_ports(active, id);
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
    struct active *active = active_index_assert(chunk, id_item(id));
    struct ports *ports = active_ports(active, id);
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
    struct active *active = active_index_assert(chunk, id_item(id));
    struct ports *ports = active_ports(active, id);
    return ports && ports->out == ITEM_NIL;
}

void chunk_ports_request(struct chunk *chunk, id_t id, enum item item)
{
    struct active *active = active_index_assert(chunk, id_item(id));
    struct ports *ports = active_ports(active, id);
    if (!ports) return;

    if (ports->in_state == ports_requested && ports->in == item) return;

    assert(ports->in_state == ports_nil);
    ports->in = item;
    ports->in_state = ports_requested;

    if (id_item(id) == ITEM_STORAGE)
        chunk->storage = ring32_push(chunk->storage, id);
    else chunk->requested = ring32_push(chunk->requested, id);
}

enum item chunk_ports_consume(struct chunk *chunk, id_t id)
{
    struct active *active = active_index_assert(chunk, id_item(id));
    struct ports *ports = active_ports(active, id);
    if (!ports) return ITEM_NIL;

    if (ports->in_state != ports_received) return ITEM_NIL;
    enum item ret = ports->in;
    ports->in = ITEM_NIL;
    ports->in_state = ports_nil;
    return ret;
}

static bool chunk_ports_step_queue(
        struct chunk *chunk, struct ring32 *requested, id_t *stop)
{
    if (ring32_empty(requested)) return false;
    if (*stop && *stop == ring32_peek(requested)) return false;

    id_t dst = ring32_pop(requested);
    if (!dst)  { chunk->workers.clean++; return true; }

    struct ports *in = active_ports(active_index_assert(chunk, id_item(dst)), dst);
    assert(in && in->in_state == ports_requested);

    struct htable_ret hret = htable_get(&chunk->provided, in->in);
    if (!hret.ok) goto nomatch;

    struct ring32 *provided = (struct ring32 *) hret.value;
    assert(provided);
    if (ring32_empty(provided)) goto nomatch;

    id_t src = ring32_pop(provided);
    if (!src) { chunk->workers.clean++; goto nomatch; }
    if (src == dst) goto nomatch; // storage can assign to self

    struct ports *out = active_ports(active_index_assert(chunk, id_item(src)), src);
    assert(out && out->out == in->in);

    out->out = ITEM_NIL;
    in->in_state = ports_received;

    chunk->workers.ops =
        vec64_append(chunk->workers.ops, ((uint64_t) src << 32) | dst);

    return true;

  nomatch:
    {
        struct ring32 *rret = ring32_push(requested, dst);
        assert(rret == requested);
        if (!*stop) *stop = dst;
        chunk->workers.fail++;
    }
    return true;
}

static void chunk_ports_step(struct chunk *chunk)
{
    chunk->workers.idle = 0;
    chunk->workers.fail = 0;
    chunk->workers.clean = 0;
    chunk->workers.queue = ring32_len(chunk->requested);
    chunk->workers.ops->len = 0;

    size_t worker = 0;

    id_t stop = 0;
    for (; worker < chunk->workers.count; ++worker) {
        if (!chunk_ports_step_queue(chunk, chunk->requested, &stop)) break;
    }

    stop = 0;
    for (; worker < chunk->workers.count; ++worker) {
        if (!chunk_ports_step_queue(chunk, chunk->storage, &stop)) break;
    }

    chunk->workers.idle = chunk->workers.count - worker;
}
