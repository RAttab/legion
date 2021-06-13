/* chunk.c
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "chunk.h"

#include "game/galaxy.h"
#include "game/active.h"
#include "utils/vec.h"
#include "utils/ring.h"
#include "utils/htable.h"


// -----------------------------------------------------------------------------
// class
// -----------------------------------------------------------------------------

typedef void (*init_fn_t) (void *state, id_t id, struct chunk *);
typedef void (*step_fn_t) (void *state, struct chunk *);

enum ports_state
{
    ports_nil = 0,
    ports_requested,
    ports_assigned,
    ports_received,
};

struct legion_packed ports
{
    item_t in;
    uint8_t in_state;
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
    cmd_fn_t cmd;

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
        .cmd = config->cmd,
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
        vec64_append(ids, make_id(class->type, i));
}

static void *class_get(struct class *class, id_t id)
{
    if (!class) return NULL;

    size_t offset = id_bot(id) * class->size;
    if (offset >= class->len) return NULL;

    return class->arena + offset;
}

static void class_create(struct class *class)
{
    class->create++;
}

static void class_grow(struct class *class)
{
    if (class->len < class->cap) return;

    if (!class->len) {
        class->cap = 1;

        class->arena = calloc(class->cap, class->size);
        class->ports = calloc(class->cap, sizeof(class->ports[0]));

        assert((uintptr_t) class->arena % s_cache_line == 0);
        assert((uintptr_t) class->ports % s_cache_line == 0);
        return;
    }

    class->cap *= 2;
    class->arena = reallocarray(class->arena, class->cap, class->size);
    class->ports = reallocarray(class->arena, class->cap, sizeof(class->ports[0]));

    size_t added = class->cap - class->len;
    memset(class->arena + class->len, 0, added * class->size);
    memset(class->ports + class->len, 0, added * sizeof(class->ports[0]));

    assert((uintptr_t) class->arena % s_cache_line == 0);
    assert((uintptr_t) class->ports % s_cache_line == 0);
}

static void class_step(struct class *class, struct chunk *chunk)
{
    if (!class) return;
    if (!class->step) return;

    void *end = class->arena + class->len * class->size;
    for (void *it = class->arena; it < end; it += class->size)
        class->step(it, chunk);

    while (class->create) {
        class_grow(class);

        id_t id = make_id(class->type, class->len);
        void *item = class->arena + (class->len * class->size);
        class->len++;

        class->init(item, id, chunk);
    }
}

static bool class_cmd(
        struct class *class, struct chunk *chunk,
        enum atom_io cmd, id_t src, id_t dst, size_t len, const word_t *args)
{
    void *state = class_get(class, dst);
    if (!state) return false;

    class->cmd(state, chunk, cmd, src, len, args);
    return true;
}

static struct ports *class_ports(struct class *class, id_t id)
{
    size_t index = id_bot(id);
    if (!class || index < class->len) return NULL;
    return &class->ports[index];
}


// -----------------------------------------------------------------------------
// chunk
// -----------------------------------------------------------------------------

struct chunk
{
    struct star star;
    struct class *class[ITEMS_ACTIVE_LEN];

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
    if (!chunk->star.elements[item]) return false;

    chunk->star.elements[item]--;
    return true;
}

struct class *chunk_class(struct chunk *chunk, item_t item)
{
    assert(item >= ITEM_ACTIVE_FIRST && item < ITEM_ACTIVE_LAST);
    return chunk->class[item - ITEM_ACTIVE_FIRST];
}

struct vec64* chunk_list(struct chunk *chunk)
{
    size_t len = 0;
    for (size_t i = 0; i < ITEMS_ACTIVE_LEN; ++i) {
        if (chunk->class[i]) len += chunk->class[i]->len;
    }

    struct vec64 *ids = vec64_reserve(len);
    for (size_t i = 0; i < ITEMS_ACTIVE_LEN; ++i)
        class_list(chunk->class[i], ids);

    return ids;
}

void *chunk_get(struct chunk *chunk, id_t id)
{
    return class_get(chunk_class(chunk, id), id);
}

void chunk_create(struct chunk *chunk, item_t item)
{
    assert(item >= ITEM_ACTIVE_FIRST && item < ITEM_ACTIVE_LAST);

    struct class **ptr = &chunk->class[item - ITEM_ACTIVE_FIRST];
    if (!*ptr) *ptr = class_alloc(item);
    class_create(*ptr);
}

void chunk_step(struct chunk *chunk)
{
    for (size_t i = 0; i < ITEMS_ACTIVE_LEN; ++i)
        class_step(chunk->class[i], chunk);
}

bool chunk_cmd(
        struct chunk *chunk,
        enum atom_io cmd, id_t src, id_t dst, size_t len, const word_t *args)
{
    struct class *class = chunk_class(chunk, id_item(dst));
    return class_cmd(class, chunk, cmd, src, dst, len, args);
}

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
        struct ring32 *provided = ring32_reserve(1);
        hret = htable_put(&chunk->provided, item, (uint64_t) provided);
        assert(hret.ok);
    }

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
    return ret;
}

void chunk_ports_give(struct chunk *chunk, id_t id, item_t item)
{
    struct ports *ports = class_ports(chunk_class(chunk, id_item(id)), id);
    if (!ports) return;

    assert(ports->in_state == ports_assigned);
    ports->in_state = ports_received;
    ports->in = item;
}

item_t chunk_ports_take(struct chunk *chunk, id_t id)
{
    struct ports *ports = class_ports(chunk_class(chunk, id_item(id)), id);
    if (!ports) return ITEM_NIL;

    item_t ret = ports->out;
    ports->out = ITEM_NIL;
    return ret;
}

bool chunk_ports_pair(struct chunk *chunk, item_t *item, id_t *src, id_t *dst)
{
    *dst = ring32_pop(chunk->requested);

    struct ports *ports = class_ports(chunk_class(chunk, id_item(*dst)), *dst);
    assert(ports);
    *item = ports->in;

    struct htable_ret hret = htable_get(&chunk->provided, *item);
    if (!hret.ok) goto nomatch;

    struct ring32 *provided = (struct ring32 *) hret.value;
    if (ring32_empty(provided)) goto nomatch;

    *src = ring32_pop(provided);
    ports->in_state = ports_assigned;
    return true;

  nomatch:
    struct ring32 *rret = ring32_push(chunk->requested, *dst);
    assert(rret == chunk->requested);
    return false;
}