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
    user_id owner;
    vm_word name;

    world_ts updated;

    struct log *log;

    // Ports
    struct htable provided;
    struct ring16 *requested;
    struct ring16 *storage;

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

struct chunk *chunk_alloc_empty(void)
{
    struct chunk *chunk = calloc(1, sizeof(*chunk));
    *chunk = (struct chunk) {
        .log = log_new(chunk_log_cap),
        .requested = ring16_reserve(16),
        .storage = ring16_reserve(1),
        .pills = ring64_reserve(2),
        .workers.ops = vec32_reserve(1),
    };

    for (size_t i = 0; i < array_len(chunk->active); ++i)
        active_init(chunk->active + i, ITEM_ACTIVE_FIRST + i);

    return chunk;
}

struct chunk *chunk_alloc(
        struct world *world, const struct star *star, user_id owner, vm_word name)
{
    assert(world);

    struct chunk *chunk = chunk_alloc_empty();
    chunk->world = world;
    chunk->star = *star;
    chunk->name = name;
    chunk->owner = owner;
    chunk->updated = world_time(world);
    return chunk;
}

void chunk_free(struct chunk *chunk)
{
    log_free(chunk->log);

    ring16_free(chunk->requested);
    ring16_free(chunk->storage);

    for (const struct htable_bucket *it = htable_next(&chunk->provided, NULL);
         it; it = htable_next(&chunk->provided, it))
        ring16_free((void *) it->value);
    htable_reset(&chunk->provided);

    ring64_free(chunk->pills);
    vec32_free(chunk->workers.ops);
    htable_reset(&chunk->listen);

    for (struct active *it = active_next(chunk, NULL);
         it; it = active_next(chunk, it))
        active_free(it);
    free(chunk);
}


// -----------------------------------------------------------------------------
// save/load
// -----------------------------------------------------------------------------

static void chunk_save_provided(struct chunk *chunk, struct save *save)
{
    save_write_value(save, chunk->provided.len);
    for (const struct htable_bucket *it = htable_next(&chunk->provided, NULL);
         it; it = htable_next(&chunk->provided, it))
    {
        save_write_value(save, (enum item) it->key);
        ring16_save((struct ring16 *) it->value, save);
    }
}

static bool chunk_load_provided(struct chunk *chunk, struct save *save)
{
    size_t len = save_read_type(save, typeof(chunk->provided.len));
    htable_reserve(&chunk->provided, len);

    for (size_t i = 0; i < len; ++i) {
        enum item item = save_read_type(save, typeof(item));

        struct ring16 *ring = ring16_load(save);
        if (!ring) return false;

        struct htable_ret ret = htable_put(&chunk->provided, item, (uintptr_t) ring);
        assert(ret.ok);
    }

    return true;
}

static void chunk_save_listen(struct chunk *chunk, struct save *save)
{
    save_write_value(save, chunk->listen.len);
    for (const struct htable_bucket *it = htable_next(&chunk->listen, NULL);
         it; it = htable_next(&chunk->listen, it))
    {
        save_write_value(save, it->key);
        save_write_value(save, it->value);
    }
}

static bool chunk_load_listen(struct chunk *chunk, struct save *save)
{
    size_t len = save_read_type(save, typeof(chunk->listen.len));
    htable_reserve(&chunk->listen, len);
    for (size_t i = 0; i < len; ++i) {
        uint64_t src = save_read_type(save, typeof(src));
        uint64_t chans = save_read_type(save, typeof(chans));

        struct htable_ret ret = htable_get(&chunk->listen, src);
        if (ret.ok)
            ret = htable_xchg(&chunk->listen, src, chans);
        else ret = htable_put(&chunk->listen, src, chans);
        assert(ret.ok);
    }

    return true;
}

static void chunk_save_active(struct chunk *chunk, struct save *save)
{
    for (struct active *it = active_next(chunk, NULL); it; it = active_next(chunk, it))
        active_save(it, save);
}

static bool chunk_load_active(struct chunk *chunk, struct save *save)
{
    for (size_t i = 0; i < array_len(chunk->active); ++i) {
        struct active *it = chunk->active + i;

        active_init(it, ITEM_ACTIVE_FIRST + i);
        if (it->skip) continue;

        if (!active_load(it, save, chunk)) return false;
    }
    return true;
}

void chunk_save(struct chunk *chunk, struct save *save)
{
    save_write_magic(save, save_magic_chunk);

    save_write_value(save, chunk->name);
    save_write_value(save, chunk->owner);
    star_save(&chunk->star, save);

    chunk_save_provided(chunk, save);
    ring16_save(chunk->requested, save);
    ring16_save(chunk->storage, save);

    ring64_save(chunk->pills, save);
    energy_save(&chunk->energy, save);
    save_write_value(save, chunk->workers.count);
    save_write_vec32(save, chunk->workers.ops);

    log_save(chunk->log, save);
    chunk_save_listen(chunk, save);
    chunk_save_active(chunk, save);

    save_write_magic(save, save_magic_chunk);
}

struct chunk *chunk_load(struct world *world, struct save *save)
{
    if (!save_read_magic(save, save_magic_chunk)) return NULL;

    struct chunk *chunk = calloc(1, sizeof(*chunk));
    chunk->world = world;

    save_read_into(save, &chunk->name);
    save_read_into(save, &chunk->owner);
    star_load(&chunk->star, save);

    if (!chunk_load_provided(chunk, save)) goto fail;
    if (!(chunk->requested = ring16_load(save))) goto fail;
    if (!(chunk->storage = ring16_load(save))) goto fail;

    if (!(chunk->pills = ring64_load(save))) goto fail;
    if (!energy_load(&chunk->energy, save)) goto fail;
    save_read_into(save, &chunk->workers.count);
    if (!save_read_vec32(save, &chunk->workers.ops)) goto fail;

    if (!(chunk->log = log_load(save))) goto fail;
    if (!chunk_load_listen(chunk, save)) goto fail;
    if (!chunk_load_active(chunk, save)) goto fail;

    if (!save_read_magic(save, save_magic_chunk)) goto fail;
    return chunk;

  fail:
    chunk_free(chunk);
    return NULL;
}


static void chunk_save_delta_provided(
        struct chunk *chunk, struct save *save, const struct chunk_ack *ack)
{
    assert(chunk->provided.len < ITEM_MAX);
    for (const struct htable_bucket *it = htable_next(&chunk->provided, NULL);
         it; it = htable_next(&chunk->provided, it))
    {
        struct htable_ret ret = htable_get(&ack->provided, it->key);
        struct ring_ack rack = ring_ack_from_u64(ret.value);

        // Optimization to avoid sending full deltas for acknowledged empty rings.
        struct ring16 *ring = (void *) it->value;
        if (rack.head == rack.tail && ring->head == ring->tail) continue;

        save_write_value(save, (enum item) it->key);
        ring16_save_delta(ring, save, &rack);
    }

    save_write_value(save, (enum item) 0);
}

static bool chunk_load_delta_provided(
        struct chunk *chunk, struct save *save, struct chunk_ack *ack)
{
    enum item item = 0;
    while ((item = save_read_type(save, typeof(item)))) {

        struct htable_ret ret_ack = htable_get(&ack->provided, item);
        struct ring_ack rack = ring_ack_from_u64(ret_ack.value);

        struct htable_ret ret = htable_get(&chunk->provided, item);
        struct ring16 *ring = (void *) ret.value;

        if (!ring16_load_delta(&ring, save, &rack)) {
            if (!ret.ok) ring16_free(ring);
            return false;
        }

        if (ret.ok)
            ret = htable_xchg(&chunk->provided, item, (uintptr_t) ring);
        else ret = htable_put(&chunk->provided, item, (uintptr_t) ring);
        assert(ret.ok);

        if (ret_ack.ok)
            ret_ack = htable_xchg(&ack->provided, item, ring_ack_to_u64(rack));
        else ret_ack = htable_put(&ack->provided, item, ring_ack_to_u64(rack));
        assert(ret_ack.ok);
    }

    return true;
}

static void chunk_save_delta_active(
        struct chunk *chunk, struct save *save, const struct chunk_ack *ack)
{
    for (struct active *it = active_next(chunk, NULL); it; it = active_next(chunk, it)) {
        hash_val hash = active_hash(it, hash_init());
        if (ack->active[it->type - ITEM_ACTIVE_FIRST] == hash) continue;

        save_write_value(save, it->type);
        save_write_value(save, hash);
        active_save(it, save);
    }
    save_write_value(save, (enum item) 0);
}

static bool chunk_load_delta_active(
        struct chunk *chunk, struct save *save, struct chunk_ack *ack)
{
    enum item type = 0;
    while ((type = save_read_type(save, typeof(type)))) {
        hash_val hash = save_read_type(save, typeof(hash));

        struct active *active = active_index(chunk, type);
        assert(active && !active->skip);

        if (!active_load(active, save, NULL)) return false;
        ack->active[type - ITEM_ACTIVE_FIRST] = hash;
    }

    return true;
}


void chunk_save_delta(
        struct chunk *chunk, struct save *save, const struct ack *ack)
{
    save_write_magic(save, save_magic_chunk);

    save_write_value(save, chunk->name);
    save_write_value(save, chunk->owner);
    star_save(&chunk->star, save);

    const struct chunk_ack *cack = &ack->chunk;
    static const struct chunk_ack cack_nil = {0};
    if (!coord_eq(ack->chunk.coord, chunk->star.coord)) cack = &cack_nil;

    chunk_save_delta_provided(chunk, save, cack);
    ring16_save_delta(chunk->requested, save, &cack->requested);
    ring16_save_delta(chunk->storage, save, &cack->storage);

    ring64_save_delta(chunk->pills, save, &cack->pills);
    energy_save(&chunk->energy, save);
    save_write_value(save, chunk->workers.count);
    save_write_vec32(save, chunk->workers.ops);

    log_save_delta(chunk->log, save, cack->time);
    chunk_save_listen(chunk, save);
    chunk_save_delta_active(chunk, save, cack);

    save_write_magic(save, save_magic_chunk);
}

bool chunk_load_delta(struct chunk *chunk, struct save *save, struct ack *ack)
{
    if (!save_read_magic(save, save_magic_chunk)) return false;

    save_read_into(save, &chunk->name);
    save_read_into(save, &chunk->owner);
    if (!star_load(&chunk->star, save)) return false;

    if (!coord_eq(ack->chunk.coord, chunk->star.coord)) ack_reset_chunk(ack);

    if (!chunk_load_delta_provided(chunk, save, &ack->chunk)) return false;
    if (!ring16_load_delta(&chunk->requested, save, &ack->chunk.requested)) return false;
    if (!ring16_load_delta(&chunk->storage, save, &ack->chunk.storage)) return false;

    if (!ring64_load_delta(&chunk->pills, save, &ack->chunk.pills)) return false;
    if (!energy_load(&chunk->energy, save)) return false;
    save_read_into(save, &chunk->workers.count);
    if (!save_read_vec32(save, &chunk->workers.ops)) return false;

    if (!log_load_delta(chunk->log, save, ack->chunk.time)) return false;
    if (!chunk_load_listen(chunk, save)) return false;
    if (!chunk_load_delta_active(chunk, save, &ack->chunk)) return false;

    if (!save_read_magic(save, save_magic_chunk)) return false;

    ack->chunk.coord = chunk->star.coord;
    return true;
}


// -----------------------------------------------------------------------------
// ops
// -----------------------------------------------------------------------------

user_id chunk_owner(struct chunk *chunk)
{
    return chunk->owner;
}

struct world *chunk_world(const struct chunk *chunk)
{
    assert(chunk->world);
    return chunk->world;
}

const struct star *chunk_star(const struct chunk *chunk)
{
    return &chunk->star;
}

struct tech *chunk_tech(const struct chunk *chunk)
{
    return world_tech(chunk_world(chunk), chunk->owner);
}

world_ts chunk_updated(const struct chunk *chunk)
{
    return chunk->updated;
}

vm_word chunk_name(struct chunk *chunk)
{
    return chunk->name;
}
void chunk_rename(struct chunk *chunk, vm_word new)
{
    chunk->name = new;
    chunk->updated = world_time(chunk->world);
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

im_id chunk_last(struct chunk *chunk, enum item item)
{
    return active_last(active_index_assert(chunk, item));
}

struct vec16* chunk_list(struct chunk *chunk)
{
    size_t sum = 0;
    for (struct active *it = active_next(chunk, NULL); it; it = active_next(chunk, it))
        sum += active_count(it);

    struct vec16 *ids = vec16_reserve(sum);
    for (struct active *it = active_next(chunk, NULL); it; it = active_next(chunk, it))
        active_list(it, ids);

    return ids;
}

struct vec16* chunk_list_filter(struct chunk *chunk, im_list filter)
{
    size_t sum = 0;
    for (im_list it = filter; *it; it++)
        sum += active_count(active_index_assert(chunk, *it));

    struct vec16 *ids = vec16_reserve(sum);
    for (im_list it = filter; *it; it++)
        active_list(active_index_assert(chunk, *it), ids);

    return ids;
}

const void *chunk_get(struct chunk *chunk, im_id id)
{
    return active_get(active_index_assert(chunk, im_id_item(id)), id);
}

bool chunk_copy(struct chunk *chunk, im_id id, void *dst, size_t len)
{
    return active_copy(active_index_assert(chunk, im_id_item(id)), id, dst, len);
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
        vm_word cargo = 0;
        chunk_lanes_arrive(chunk, item, coord_nil(), &cargo, 1);
        return true;
    }

    default: { assert(false); }
    }
}

bool chunk_create(struct chunk *chunk, enum item item)
{
    if (chunk_create_logistics(chunk, item)) return true;
    return active_create(active_index_assert(chunk, item));
}

bool chunk_create_from(
        struct chunk *chunk, enum item item, const vm_word *data, size_t len)
{

    if (chunk_create_logistics(chunk, item)) { assert(!len); return true; }
    return active_create_from(active_index_assert(chunk, item), chunk, data, len);
}

bool chunk_delete(struct chunk *chunk, im_id id)
{
    return active_delete(active_index_assert(chunk, im_id_item(id)), id);
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
        enum io io, im_id src, im_id dst, const vm_word *args, size_t len)
{
    struct active *active = active_index(chunk, im_id_item(dst));
    if (!active) return false;

    return active_io(active, chunk, io, src, dst, args, len);
}


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

void chunk_log(struct chunk *chunk, im_id id, vm_word key, vm_word value)
{
    assert(chunk->world);

    struct coord star = chunk->star.coord;
    log_push(chunk->log, world_time(chunk->world), star, id, key, value);
    world_log_push(chunk->world, chunk->owner, star, id, key, value);
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
        struct chunk *chunk, im_id id, struct coord src, uint8_t chan)
{
    assert(chan < im_channels_max);

    const uint64_t key = coord_to_u64(src);
    struct htable_ret ret = htable_get(&chunk->listen, key);

    struct im_channels channels = im_channels_from_u64(ret.value);
    if (!channels.c[chan]) channels.c[chan] = id;
    uint64_t value = im_channels_as_u64(channels);

    if (ret.ok)
        ret = htable_xchg(&chunk->listen, key, value);
    else ret = htable_put(&chunk->listen, key, value);
    assert(ret.ok);
}

void chunk_lanes_unlisten(
        struct chunk *chunk, im_id id, struct coord src, uint8_t chan)
{
    assert(chan < im_channels_max);

    const uint64_t key = coord_to_u64(src);
    struct htable_ret ret = htable_get(&chunk->listen, key);
    if (!ret.ok) return;

    struct im_channels channels = im_channels_from_u64(ret.value);
    if (channels.c[chan] != id) return;

    channels.c[chan] = 0;
    uint64_t value = im_channels_as_u64(channels);

    if (value)
        ret = htable_xchg(&chunk->listen, key, value);
    else ret = htable_del(&chunk->listen, key);
    assert(ret.ok);
}

static void chunk_lanes_receive(
        struct chunk *chunk, struct coord src, const vm_word *data, size_t len)
{
    assert(len >= 1);

    uint8_t packet_chan = 0, packet_len = 0;
    im_packet_unpack(data[0], &packet_chan, &packet_len);

    struct htable_ret ret = htable_get(&chunk->listen, coord_to_u64(src));
    if (!ret.ok) return;

    struct im_channels channels = im_channels_from_u64(ret.value);
    im_id dst = channels.c[packet_chan];
    if (!dst) return;

    chunk_io(chunk, IO_RECV, 0, dst, data, len);
}

void chunk_lanes_arrive(
        struct chunk *chunk,
        enum item item, struct coord src,
        const vm_word *data, size_t len)
{
    switch (item)
    {

    case ITEM_ACTIVE_FIRST...ITEM_ACTIVE_LAST: {
        if (!chunk_create_from(chunk, item, data, len))
            chunk_log(chunk, make_im_id(item, 0), IO_ARRIVE, IOE_OUT_OF_SPACE);
        break;
    }

    case ITEM_PILL: {
        vm_word cargo = len == 1 ? data[0] : 0;
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

bool chunk_lanes_dock(struct chunk *chunk, vm_word *data)
{
    if (ring64_empty(chunk->pills)) return false;
    *data = ring64_pop(chunk->pills);
    return true;
}

void chunk_lanes_launch(
        struct chunk *chunk,
        enum item item, size_t speed,
        struct coord dst,
        const vm_word *data, size_t len)
{
    assert(chunk->world);

    switch (item)
    {
    case ITEM_ACTIVE_FIRST...ITEM_ACTIVE_LAST: { break; }
    case ITEM_PILL: { break; }
    case ITEM_DATA: { break; }
    default: { assert(false); }
    }

    world_lanes_launch(
            chunk->world,
            chunk->owner, item, speed,
            chunk->star.coord, dst,
            data, len);
}


// -----------------------------------------------------------------------------
// ports
// -----------------------------------------------------------------------------

static void ring16_replace(struct ring16 *ring, uint32_t old, uint32_t new)
{
    for (size_t i = 0; i < ring->cap; ++i) {
        if (ring->vals[i] == old) ring->vals[i] = new;
    }
}

void chunk_ports_reset(struct chunk *chunk, im_id id)
{
    struct active *active = active_index_assert(chunk, im_id_item(id));
    struct ports *ports = active_ports(active, id);
    if (!ports) return;

    if (ports->in_state == ports_requested) {
        if (im_id_item(id) == ITEM_STORAGE)
            ring16_replace(chunk->storage, id, 0);
        else
            ring16_replace(chunk->requested, id, 0);
    }

    if (ports->out) {
        struct htable_ret ret = htable_get(&chunk->provided, ports->out);
        assert(ret.ok);

        ring16_replace((struct ring16 *) ret.value, id, 0);
    }

    *ports = (struct ports) {0};
}

bool chunk_ports_produce(struct chunk *chunk, im_id id, enum item item)
{
    struct active *active = active_index_assert(chunk, im_id_item(id));
    struct ports *ports = active_ports(active, id);
    if (!ports) return false;

    if (ports->out != ITEM_NIL) return false;
    ports->out = item;

    struct ring16 *provided = NULL;
    struct htable_ret hret = htable_get(&chunk->provided, item);

    if (likely(hret.ok)) provided = (struct ring16 *) hret.value;
    else {
        provided = ring16_reserve(1);
        hret = htable_put(&chunk->provided, item, (uint64_t) provided);
        assert(hret.ok);
    }
    assert(provided);

    struct ring16 *rret = ring16_push(provided, id);
    if (unlikely(rret != provided)) {
        hret = htable_xchg(&chunk->provided, item, (uint64_t) rret);
        assert(hret.ok);
    }

    return true;
}

bool chunk_ports_consumed(struct chunk *chunk, im_id id)
{
    struct active *active = active_index_assert(chunk, im_id_item(id));
    struct ports *ports = active_ports(active, id);
    return ports && ports->out == ITEM_NIL;
}

void chunk_ports_request(struct chunk *chunk, im_id id, enum item item)
{
    struct active *active = active_index_assert(chunk, im_id_item(id));
    struct ports *ports = active_ports(active, id);
    if (!ports) return;

    if (ports->in_state == ports_requested && ports->in == item) return;

    assert(ports->in_state == ports_nil);
    ports->in = item;
    ports->in_state = ports_requested;

    if (im_id_item(id) == ITEM_STORAGE)
        chunk->storage = ring16_push(chunk->storage, id);
    else chunk->requested = ring16_push(chunk->requested, id);
}

enum item chunk_ports_consume(struct chunk *chunk, im_id id)
{
    struct active *active = active_index_assert(chunk, im_id_item(id));
    struct ports *ports = active_ports(active, id);
    if (!ports) return ITEM_NIL;

    if (ports->in_state != ports_received) return ITEM_NIL;
    enum item ret = ports->in;
    ports->in = ITEM_NIL;
    ports->in_state = ports_nil;
    return ret;
}

static bool chunk_ports_step_queue(
        struct chunk *chunk, struct ring16 *requested, im_id *stop)
{
    if (ring16_empty(requested)) return false;
    if (*stop && *stop == ring16_peek(requested)) return false;

    im_id dst = ring16_pop(requested);
    if (!dst)  { chunk->workers.clean++; return true; }

    struct ports *in = active_ports(active_index_assert(chunk, im_id_item(dst)), dst);
    assert(in && in->in_state == ports_requested);

    struct htable_ret hret = htable_get(&chunk->provided, in->in);
    if (!hret.ok) goto nomatch;

    struct ring16 *provided = (struct ring16 *) hret.value;
    assert(provided);
    if (ring16_empty(provided)) goto nomatch;

    im_id src = ring16_pop(provided);
    if (!src) { chunk->workers.clean++; goto nomatch; }

    // Moving to and from storage just adds noise.
    if (im_id_item(src) == ITEM_STORAGE && im_id_item(dst) == ITEM_STORAGE) {
        ring16_push(provided, src);
        goto nomatch;
    }

    struct ports *out = active_ports(active_index_assert(chunk, im_id_item(src)), src);
    assert(out && out->out == in->in);

    out->out = ITEM_NIL;
    in->in_state = ports_received;

    chunk->workers.ops =
        vec32_append(chunk->workers.ops, ((uint32_t) src << 16) | dst);

    return true;

  nomatch:
    {
        struct ring16 *rret = ring16_push(requested, dst);
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
    chunk->workers.queue = ring16_len(chunk->requested);
    chunk->workers.ops->len = 0;

    size_t worker = 0;

    im_id stop = 0;
    for (; worker < chunk->workers.count; ++worker) {
        if (!chunk_ports_step_queue(chunk, chunk->requested, &stop)) break;
    }

    stop = 0;
    for (; worker < chunk->workers.count; ++worker) {
        if (!chunk_ports_step_queue(chunk, chunk->storage, &stop)) break;
    }

    chunk->workers.idle = chunk->workers.count - worker;
}
