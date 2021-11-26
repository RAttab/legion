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

#define ringx_fn_concat(prefix, suffix) prefix ## _ ## suffix
#define ringx_fn_eval(prefix, suffix) ringx_fn_concat(prefix, suffix)
#define ringx_fn(name) ringx_fn_eval(ringx_name, name)


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
    if (ringx_fn(empty)(ring)) return 0;
    return likely(ring->tail < ring->head) ?
        ring->head - ring->tail :
        (ring->head + 1) + (UINT16_MAX - ring->tail);
}

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

inline struct ringx_name *ringx_fn(push) (struct ringx_name *ring, ringx_type val)
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

#undef ringx_fn_concat
#undef ringx_fn_eval
#undef ringx_fn

#undef ringx_type
#undef ringx_name
