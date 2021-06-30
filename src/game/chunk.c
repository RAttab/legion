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


// -----------------------------------------------------------------------------
// class
// -----------------------------------------------------------------------------

typedef void (*init_fn_t) (void *state, id_t id, struct chunk *);
typedef void (*step_fn_t) (void *state, struct chunk *);

enum legion_packed ports_state
{
    ports_nil = 0,
    ports_requested,
    ports_received,
};

struct legion_packed ports
{
    item_t in;
    enum ports_state in_state;
    item_t out;
    legion_pad(1);
};

static_assert(sizeof(struct ports) == 4);


struct class
{
    item_t type;
    uint8_t size;
    uint8_t create;
    legion_pad(2);

    uint16_t len, cap;
    legion_pad(4);

    init_fn_t init;
    step_fn_t step;
    io_fn_t io;

    void *arena;
    struct ports *ports;

    legion_pad(8);
};
static_assert(sizeof(struct class) <= s_cache_line);


static struct class *class_alloc(item_t type)
{
    const struct item_config *config = item_config(type);

    struct class *class = calloc(1, sizeof(*class));
    *class = (struct class) {
        .type = type,
        .size = config->size,
        .init = config->init,
        .step = config->step,
        .io = config->io,
    };

    return class;
}

static void class_free(struct class *class)
{
    if (!class) return;
    free(class->arena);
    free(class->ports);
}

static void class_list(struct class *class, struct vec64 *ids)
{
    if (!class) return;

    for (size_t i = 0; i < class->len; ++i)
        vec64_append(ids, make_id(class->type, i+1));
}

static void *class_get(struct class *class, id_t id)
{
    if (!class) return NULL;

    size_t index = id_bot(id)-1;
    if (index >= class->len) return NULL;

    return class->arena + (index * class->size);
}

static struct ports *class_ports(struct class *class, id_t id)
{
    size_t index = id_bot(id)-1;
    if (!class || index >= class->len) return NULL;
    return &class->ports[index];
}

static bool class_copy(struct class *class, id_t id, void *dst, size_t len)
{
    if (!class) return false;
    assert(len >= class->size);

    size_t index = id_bot(id)-1;
    if (index >= class->len) return false;

    memcpy(dst, class->arena + (index * class->size), class->size);
    return true;
}

static void class_create(struct class *class)
{
    class->create++;
}

// \todo calloc and reallocarray won't cache align anything so gotta do it
// manually.
static void class_grow(struct class *class)
{
    if (class->len < class->cap) return;

    if (!class->len) {
        class->cap = 1;
        class->arena = calloc(class->cap, class->size);
        class->ports = calloc(class->cap, sizeof(class->ports[0]));
        return;
    }

    class->cap *= 2;
    class->arena = reallocarray(class->arena, class->cap, class->size);
    class->ports = reallocarray(class->ports, class->cap, sizeof(class->ports[0]));

    size_t added = class->cap - class->len;
    memset(class->arena + (class->len * class->size), 0, added * class->size);
    memset(class->ports + class->len, 0, added * sizeof(class->ports[0]));
}

static void class_step(struct class *class, struct chunk *chunk)
{
    if (!class) return;

    if (class->step) {
        void *end = class->arena + class->len * class->size;
        for (void *it = class->arena; it < end; it += class->size)
            class->step(it, chunk);
    }

    while (class->create) {
        class_grow(class);

        id_t id = make_id(class->type, class->len+1);
        void *item = class->arena + (class->len * class->size);
        class->len++;

        class->init(item, id, chunk);
        class->create--;
    }
}

static bool class_io(
        struct class *class, struct chunk *chunk,
        enum atom_io io, id_t src, id_t dst, size_t len, const word_t *args)
{
    void *state = class_get(class, dst);
    if (!state) return false;

    class->io(state, chunk, io, src, len, args);
    return true;
}


// -----------------------------------------------------------------------------
// chunk
// -----------------------------------------------------------------------------

static void chunk_ports_step(struct chunk *);


struct chunk
{
    struct star star;
    struct class *class[ITEMS_ACTIVE_LEN];

    struct workers workers;
    struct htable provided;
    struct ring32 *requested;
};

struct chunk *chunk_alloc(const struct star *star)
{
    struct chunk *chunk = calloc(1, sizeof(*chunk));
    chunk->star = *star;
    chunk->requested = ring32_reserve(16);
    htable_reset(&chunk->provided);
    return chunk;
}

void chunk_free(struct chunk *chunk)
{
    for (size_t i = 0; i < ITEMS_ACTIVE_LEN; ++i)
        class_free(chunk->class[i]);
    free(chunk);
}

struct star *chunk_star(struct chunk *chunk)
{
    return &chunk->star;
}

bool chunk_harvest(struct chunk *chunk, item_t item)
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

struct class *chunk_class(struct chunk *chunk, item_t item)
{
    assert(item >= ITEM_ACTIVE_FIRST && item < ITEM_ACTIVE_LAST);
    return chunk->class[item - ITEM_ACTIVE_FIRST];
}

struct vec64* chunk_list(struct chunk *chunk)
{
    size_t sum = 0;
    for (size_t i = 0; i < ITEMS_ACTIVE_LEN; ++i) {
        if (chunk->class[i]) sum += chunk->class[i]->len;
    }

    struct vec64 *ids = vec64_reserve(sum);
    for (size_t i = 0; i < ITEMS_ACTIVE_LEN; ++i)
        class_list(chunk->class[i], ids);

    return ids;
}

struct vec64* chunk_list_filter(
        struct chunk *chunk, const item_t *filter, size_t len)
{
    size_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        struct class *class = chunk_class(chunk, filter[i]);
        if (class) sum += class->len;
    }

    struct vec64 *ids = vec64_reserve(sum);
    for (size_t i = 0; i < len; ++i)
        class_list(chunk_class(chunk, filter[i]), ids);

    return ids;
}

void *chunk_get(struct chunk *chunk, id_t id)
{
    return class_get(chunk_class(chunk, id_item(id)), id);
}

bool chunk_copy(struct chunk *chunk, id_t id, void *dst, size_t len)
{
    return class_copy(chunk_class(chunk, id_item(id)), id, dst, len);
}

void chunk_create(struct chunk *chunk, item_t item)
{
    if (item == ITEM_WORKER) { chunk->workers.count++; return; }
    assert(item >= ITEM_ACTIVE_FIRST && item < ITEM_ACTIVE_LAST);

    struct class **ptr = &chunk->class[item - ITEM_ACTIVE_FIRST];
    if (!*ptr) *ptr = class_alloc(item);
    class_create(*ptr);
}

void chunk_step(struct chunk *chunk)
{
    for (size_t i = 0; i < ITEMS_ACTIVE_LEN; ++i)
        class_step(chunk->class[i], chunk);
    chunk_ports_step(chunk);
}

bool chunk_io(
        struct chunk *chunk,
        enum atom_io io, id_t src, id_t dst, size_t len, const word_t *args)
{
    struct class *class = chunk_class(chunk, id_item(dst));
    return class_io(class, chunk, io, src, dst, len, args);
}

ssize_t chunk_scan(struct chunk *chunk, item_t item)
{
    switch (item) {

    case ITEM_WORKER: { return chunk->workers.count; }

    case ITEM_NATURAL_FIRST...ITEM_SYNTH_FIRST: {
        return chunk->star.elems[item - ITEM_NATURAL_FIRST];
    }

    case ITEM_ACTIVE_FIRST...ITEM_ACTIVE_LAST: {
        struct class *class = chunk_class(chunk, item);
        return class ? class->len : 0;
    }

    default: { return -1; }

    }
}


// -----------------------------------------------------------------------------
// ports
// -----------------------------------------------------------------------------

void chunk_ports_reset(struct chunk *chunk, id_t id)
{
    struct ports *ports = class_ports(chunk_class(chunk, id_item(id)), id);
    if (!ports) return;

    *ports = (struct ports) {0};
}

bool chunk_ports_produce(struct chunk *chunk, id_t id, item_t item)
{
    struct ports *ports = class_ports(chunk_class(chunk, id_item(id)), id);
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

void chunk_ports_request(struct chunk *chunk, id_t id, item_t item)
{
    struct ports *ports = class_ports(chunk_class(chunk, id_item(id)), id);
    if (!ports) return;

    assert(ports->in_state == ports_nil);
    ports->in = item;
    ports->in_state = ports_requested;

    ring32_push(chunk->requested, id);
}

item_t chunk_ports_consume(struct chunk *chunk, id_t id)
{
    struct ports *ports = class_ports(chunk_class(chunk, id_item(id)), id);
    if (!ports) return ITEM_NIL;

    if (ports->in_state != ports_received) return ITEM_NIL;
    item_t ret = ports->in;
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
