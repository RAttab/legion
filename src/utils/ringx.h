/* ringx.h
   Rémi Attab (remi.attab@gmail.com), 26 Nov 2021
   FreeBSD-style copyright and disclaimer apply
*/

#ifndef ringx_type
# error "ringx_type must be declared when including ringx.h"
#endif

#ifndef ringx_name
# error "ringx_name must be declared when including ringx.h"
#endif

#define ringx_concat_(prefix, suffix) prefix ## _ ## suffix
#define ringx_concat(prefix, suffix) ringx_concat_(prefix, suffix)

#define ringx_fn(name) ringx_concat(ringx_name, name)


#include "common.h"


// -----------------------------------------------------------------------------
// ringx
// -----------------------------------------------------------------------------

struct ringx_name
{
    ring_it_t head, tail;
    ring_it_t cap; legion_pad(4);
    ringx_type vals[];
};


inline void ringx_fn(free) (struct ringx_name *ring) { free(ring); }

inline size_t ringx_fn(cap) (const struct ringx_name *ring)
{
    return ring ? ring->cap : 0;
}

inline bool ringx_fn(empty) (const struct ringx_name *ring)
{
    return ring->head == ring->tail;
}

inline size_t ringx_fn(len) (const struct ringx_name *ring)
{
    return ring ? ring_delta(ring->tail, ring->head) : 0;
}

// -----------------------------------------------------------------------------
// ops
// -----------------------------------------------------------------------------

inline struct ringx_name *ringx_fn(reserve) (size_t size)
{
    size_t cap = 1;
    while (cap < size) cap *= 2;
    assert(cap <= ring_cap);

    struct ringx_name *ring = calloc(1, sizeof(*ring) + cap * sizeof(ring->vals[0]));
    ring->cap = cap;
    return ring;
}

inline ringx_type ringx_fn(peek) (const struct ringx_name *ring)
{
    if (ringx_fn(empty)(ring)) return 0;
    return ring->vals[ring->tail % ring->cap];
}

inline ringx_type ringx_fn(pop) (struct ringx_name *ring)
{
    if (ringx_fn(empty)(ring)) return 0;
    ringx_type val = legion_xchg(ring->vals + (ring->tail % ring->cap), 0);
    ring->tail++;
    return val;
}

static legion_unused
struct ringx_name *ringx_fn(grow) (struct ringx_name *old, size_t size)
{
    if (likely(old && size <= old->cap)) return old;

    struct ringx_name *new = ringx_fn(reserve)(size);
    if (!old) return new;

    new->head = old->head;
    new->tail = old->tail;

    size_t len = ringx_fn(len)(old);
    for (size_t i = 0; i < len; ++i) {
        new->vals[(new->tail + i) % new->cap] =
            old->vals[(old->tail + i) % old->cap];
    }

    ringx_fn(free)(old);
    return new;
}

static legion_unused
struct ringx_name *ringx_fn(push) (struct ringx_name *ring, ringx_type val)
{
    if (unlikely(ringx_fn(len)(ring) == ring->cap))
        ring = ringx_fn(grow)(ring, ring->cap + 1);

    ring->vals[ring->head % ring->cap] = val;
    ring->head++;
    return ring;
}


// -----------------------------------------------------------------------------
// save
// -----------------------------------------------------------------------------

#ifdef ringx_save
#include "game/save.h"

#define ringx_magic ringx_concat(save_magic, ringx_name)

static legion_unused
void ringx_fn(save) (const struct ringx_name *ring, struct save *save)
{
    save_write_magic(save, ringx_magic);

    save_write_value(save, ring->head);
    save_write_value(save, ring->tail);

    size_t len = ringx_fn(len)(ring);
    for (size_t i = 0; i < len; ++i)
        save_write_value(save, ring->vals[(ring->tail + i) % ring->cap]);

    save_write_magic(save, ringx_magic);
}

static legion_unused
struct ringx_name *ringx_fn(load) (struct save *save)
{
    if (!save_read_magic(save, ringx_magic)) return NULL;

    ring_it_t head = save_read_type(save, typeof(head));
    ring_it_t tail = save_read_type(save, typeof(tail));

    size_t len = ring_delta(tail, head);
    struct ringx_name *ring = ringx_fn(reserve)(len);
    ring->head = ring->tail = tail;

    for (size_t i = 0; i < len; ++i) {
        struct ringx_name *new =
            ringx_fn(push)(ring, save_read_type(save, ringx_type));
        assert(new == ring);
    }

    assert(ring->head == head);
    assert(ring->tail == tail);
    if (!save_read_magic(save, ringx_magic)) goto fail;
    return ring;

  fail:
    ringx_fn(free)(ring);
    return NULL;
}


static legion_unused
void ringx_fn(save_delta) (
        const struct ringx_name *ring,
        struct save *save,
        const struct ring_ack *ack)
{
    save_write_magic(save, ringx_magic);

    save_write_value(save, ack->head);
    save_write_value(save, ack->tail);
    save_write_value(save, ring->head);
    save_write_value(save, ring->tail);

    bool all =
        (ring_delta(ring->tail, ack->head) > ring_cap) ||
        (ring_delta(ack->head, ring->head) > ring_cap) ||
        (ring_delta(ack->tail, ring->tail) > ring_cap);

    ring_it_t start = all ? ring->tail : ack->head;
    size_t len = ring_delta(start, ring->head);
    assert(len <= ring_cap);

    for (size_t i = 0; i < len; ++i)
        save_write_value(save, ring->vals[(start + i) % ring->cap]);

    save_write_magic(save, ringx_magic);
}

static legion_unused
bool ringx_fn(load_delta) (
        struct ringx_name **ret,
        struct save *save,
        struct ring_ack *ack)
{
    if (!save_read_magic(save, ringx_magic)) return false;

    ring_it_t head_ack = save_read_type(save, typeof(head_ack));
    ring_it_t tail_ack = save_read_type(save, typeof(tail_ack));
    ring_it_t head = save_read_type(save, typeof(head));
    ring_it_t tail = save_read_type(save, typeof(tail));

    bool all =
        (ring_delta(tail, head_ack) > ring_cap) ||
        (ring_delta(head_ack, head) > ring_cap) ||
        (ring_delta(tail_ack, tail) > ring_cap);

    ring_it_t start = all ? tail : head_ack;
    size_t len = ring_delta(start, head);
    assert(len < ring_cap);

    struct ringx_name *ring = ringx_fn(grow)(*ret, len);
    ring->head = start;
    ring->tail = tail;
    for (size_t i = 0; i < len; ++i)
        ring = ringx_fn(push)(ring, save_read_type(save, ringx_type));
    assert(ring->head == head);
    assert(ring->tail == tail);

    *ret = ring;
    ack->head = ring->head;
    ack->tail = ring->tail;
    return save_read_magic(save, ringx_magic);
}

#undef ringx_magic
#endif // ringx_save


// -----------------------------------------------------------------------------
// footer
// -----------------------------------------------------------------------------

#undef ringx_concat_
#undef ringx_concat
#undef ringx_fn

#undef ringx_type
#undef ringx_name
#undef ringx_save
