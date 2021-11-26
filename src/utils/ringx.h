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
    uint16_t head, tail;
    uint16_t cap; legion_pad(2);
    ringx_type vals[];
};


inline void ringx_fn(free) (struct ringx_name *ring) { free(ring); }

inline size_t ringx_fn(cap) (struct ringx_name *ring)
{
    return ring ? ring->cap : 0;
}

inline bool ringx_fn(empty) (struct ringx_name *ring)
{
    return ring->head == ring->tail;
}

inline size_t ringx_fn(len) (struct ringx_name *ring)
{
    return ring ? ring_len(ring->head, ring->tail) : 0;
}

// -----------------------------------------------------------------------------
// ops
// -----------------------------------------------------------------------------


inline struct ringx_name *ringx_fn(reserve) (size_t size)
{
    size_t cap = 1;
    while (cap < size) cap *= 2;

    struct ringx_name *ring = calloc(1, sizeof(*ring) + cap * sizeof(ring->vals[0]));
    ring->cap = cap;
    return ring;
}

inline ringx_type ringx_fn(peek) (struct ringx_name *ring)
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
struct ringx_name *ringx_fn(push) (struct ringx_name *ring, ringx_type val)
{
    size_t len = ringx_fn(len)(ring);
    if (unlikely(len == ring->cap)) {
        assert(ring->cap <= UINT16_MAX);

        struct ringx_name *new = ringx_fn(reserve)(ring->cap + 1);
        new->head = ring->head;
        new->tail = ring->tail;

        for (size_t i = 0; i < len; ++i) {
            new->vals[(new->tail + i) % new->cap] =
                ring->vals[(ring->tail + i) % ring->cap];
        }

        free(ring);
        ring = new;
    }

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
void ringx_fn(save) (struct ringx_name *ring, struct save *save)
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

    uint16_t head = save_read_type(save, typeof(head));
    uint16_t tail = save_read_type(save, typeof(tail));

    size_t len = ring_len(head, tail);
    struct ringx_name *ring = ringx_fn(reserve)(len);
    ring->head = ring->tail = tail;

    static_assert(sizeof(head) == sizeof(ring->head));
    static_assert(sizeof(tail) == sizeof(ring->tail));
    
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
