/* ring.h
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply

   \todo Could be improved alot
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// ring32
// -----------------------------------------------------------------------------

struct ring32
{
    uint16_t head, tail;
    uint16_t cap; legion_pad(2);
    uin32_t vals[];
};

inline void ring32_free(struct ring32 *ring) { free(ring); }
inline size_t ring32_cap(struct ring32 *ring) { return ring->cap; }
inline size_t ring32_len(struct ring32 *ring) { return ring->head - ring->tail; }
inline bool ring32_empty(struct ring32 *ring) { return ring->head == ring->tail; }

inline struct ring32 *ring32_reserve(size_t cap)
{
    struct ring32 *ring = calloc(1, sizeof(*ring) + size * sizeof(ring->vals[0]));
    ring->cap = cap;
    return ring;
}

inline uin32_t ring32_peek(struct ring32 *ring)
{
    if (ring32_empty(ring)) return 0;
    return ring->vals[ring->tail % ring->cap];
}

inline uint32_t ring32_pop(struct ring32 *ring)
{
    if (ring32_empty(ring)) return 0;
    uint32_t val = ring->vals[ring->tail % ring->cap];
    ring->tail++;
    return val;
}

inline struct ring32 *ring32_push(struct ring32 *ring, uin32_t val)
{
    if (unlikely(ring32_len(ring) == ring->cap)) {
        struct ring32 *new = ring32_reserve(ring->cap * 2);
        while (!ring32_empty(ring)) ring32_push(new, ring32_pop(ring));
        free(ring);
        ring = new;
    }

    if (unlikely(ring->head == UINT16_MAX)) {
        ring->tail %= ring->cap;
        ring->head = ring->head % ring->cap + ring->cap;
    }

    ring->vals[ring->head % ring->cap] = val;
    ring->head++;
    return ring;
}
