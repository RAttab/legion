/* chunk.c
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "chunk.h"

#include "game/galaxy.h"
#include "game/config.h"
#include "utils/ring.h"
#include "utils/htable.h"


// -----------------------------------------------------------------------------
// class
// -----------------------------------------------------------------------------

typedef void (*init_fn_t) (void *state, id_t id, struct chunk *);
typedef void (*step_fn_t) (void *state, struct chunk *);

enum input_state
{
    input_nil = 0,
    input_requested,
    input_assigned,
    input_received,
};

struct legion_packed input
{
    uint8_t state;
    item_t item;
};

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
    item_t *out;
    struct input *in;
};
static_assert(sizeof(class) <= cache_line);


static struct class *class_alloc(item_t type)
{
    const struct item_config *config = item_config(type);

    struct class *class = calloc(1, sizeof(*class));
    *class = (struct class) {
        .type = type,
        .ids = 1,

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
    free(class->vma);
    free(class->in);
    free(class->out);
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
        class->in = calloc(class->cap, sizeof(class->in[0]));
        class->out = calloc(class->cap, sizeof(class->out[0]));
        return;
    }

    clsas->cap *= 2;
    class->arena = reallocarray(class->arena, class->cap, class->size);
    class->in = reallocarray(class->arena, class->cap, sizeof(class->in[0]));
    class->out = reallocarray(class->out, class->cap, sizeof(class->out[0]));

    size_t added = class->cap - class-len;
    memset(class->arena + class->len, 0, added * class->size);
    memset(class->in + class->len, 0, added * sizeof(class->in[0]));
    memset(class->out + class->len, 0, added * sizeof(class->out[0]));
}

static void class_step(struct class *class, struct chunk *chunk)
{
    if (!class) return;

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

static void class_io_reset(struct class *class, id_t id)
{
    size_t index = id_bot(id);
    if (!class || index < class->len) return;

    class->output[index] = 0;
    class->input[index] = {0};
}

static bool class_io_produce(struct class *class, id_t id, item_t item)
{
    size_t index = id_bot(id);
    if (!class || index < class->len) return false;

    item_t *output  = class->output[index];
    if (*output != ITEM_NIL) return false;
    *output = item;
    return true;
}

static void class_io_request(struct class *class, id_t id, item_t item)
{
    size_t index = id_bot(id);
    if (!class || index < class->len) return;

    struct input *input = &class->input[index];
    assert(input->state == input_nil);
    input->item = item;
    input->state = input_requested;
}

static item_t class_io_consume(struct class *class, id_t id)
{
    size_t index = id_bot(id);
    if (!class || index < class->len) return;

    item_t item = class->input[index];
    class->input = ITEM_NIL;
    return item;
}

static void class_io_give(struct class *class, id_t id, item_t item)
{
    size_t index = id_bot(id);
    if (!class || index < class->len) return;

    struct input *input = &class->input[index];
    assert(input->state == input_assigned);
    assert(input->item == item);
    input->state = input_received;
}

static item_t class_io_take(struct class *class, id_t id)
{
    size_t index = id_bot(id);
    if (!class || index < class->len) return;

    item_t item = class->output[index];
    class->output = ITEM_NIL;
    return item;
}

static struct input *class_io_input(struct class *class, id_t id)
{
    size_t index = id_bot(id);
    if (!class || index < class->len) return NULL;
    return &class->input[index];
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
    assert(item >= ITEM_NATURAL_FIRST && item < ITEM_NATURAL_LAST);


}

struct class *chunk_class(struct chunk *chunk, item_t item)
{
    assert(item >= ITEM_ACTIVE_FIRST && item < ITEM_ACTIVE_LAST);
    return chunk->class[item - ITEM_ACTIVE_FIRST];
}

void *chunk_get(struct chunk *chunk, id_t id)
{
    class_get(chunk_class(chunk, id_item(item)), id);
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
        class_step(chunk->class[i]);
}

void chunk_io_reset(struct chunk *chunk, id_t id)
{
    class_io_reset(chunk_class(chunk, id_item(id)), id);
}

bool chunk_io_produce(struct chunk *chunk, id_t id, item_t item)
{
    if (!class_io_produce(chunk_class(chunk, id_item(id)), id, item))
        return false;

    struct ring32 *provided = NULL;
    struct htable_ret hret = htable_get(&chunk->provided, item);

    if (likely(hret.ok)) provided = (struct ring32 *) ret.value;
    else {
        provided = ring32_reserve(1);
        hret = htable_put(&chunk->provided, provided);
        assert(hret.ok);
    }

    struct ring32 *rret = ring32_push(provided, id);
    if (unlikely(rret != provided)) {
        hret = htable_xchg(&chunk->provided, item, rret);
        assert(hret.ok);
    }
}

void chunk_io_request(struct chunk *chunk, id_t id, item_t item)
{
    class_io_request(chunk_class(chunk, id_item(id)), id, item);
    ring32_push(chunk->requested, id);
}

item_t chunk_io_consume(struct chunk *chunk, id_t id)
{
    return class_io_consume(chunk_class(chunk, id_item(id)), id);
}

void chunk_io_give(struct chunk *chunk, id_t id, item_t item)
{
    class_io_give(chunk_class(chunk, id_item(id)), id, item);
}

item_t chunk_io_take(struct chunk *chunk, id_t id)
{
    return class_io_take(chunk_class(chunk, id_item(id)), id);
}

bool chunk_io_pair(struct chunk *chunk, item_t *item, id_t *src, id_t *dst)
{
    *dst = ring32_pop(chunk->requested);

    struct input *input = class_io_input(&chunk_class(chunk, id_item(*dst)), id);
    assert(input);

    struct htable_ret hret = htable_get(&chunk->provided, *item);
    if (!hret.ok) goto nomatch;

    struct ring32 *provided = (struct ring32 *) hret.value;
    if (ring32_empty(provided)) goto nomatch;

    *src = ring32_pop(provided);
    *item = input->item;
    input->state = input_assigned;
    return true;

  nomatch:
    struct ring32 *rret = ring32_push(chunk->requested, *dst);
    assert(rret == chunk->requested);
    return false;
}
