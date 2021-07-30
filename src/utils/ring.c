/* ring.c
   Rémi Attab (remi.attab@gmail.com), 30 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "utils/ring.h"

// -----------------------------------------------------------------------------
// ring32
// -----------------------------------------------------------------------------

struct ring32 *ring32_push(struct ring32 *ring, uint32_t val)
{
    if (unlikely(ring32_len(ring) == ring->cap)) {
        struct ring32 *new = ring32_reserve(ring->cap ? ring->cap * 2 : 1);
        while (!ring32_empty(ring)) ring32_push(new, ring32_pop(ring));
        free(ring);
        ring = new;
    }

    if (unlikely(ring->head == UINT16_MAX)) {
        dbg0("ring.limit");
        ring->tail %= ring->cap;
        ring->head %= ring->cap;
    }

    ring->vals[ring->head % ring->cap] = val;
    ring->head++;
    return ring;
}

size_t ring32_replace(struct ring32 *ring, uint32_t old, uint32_t new)
{
    size_t n = 0;
    for (size_t i = 0; i < ring->cap; ++i) {
        if (ring->vals[i] == old) { ring->vals[i] = new; n++; }
    }
    return n;
}


// -----------------------------------------------------------------------------
// ring64
// -----------------------------------------------------------------------------

struct ring64 *ring64_push(struct ring64 *ring, uint64_t val)
{
    if (unlikely(ring64_len(ring) == ring->cap)) {
        struct ring64 *new = ring64_reserve(ring->cap * 2);
        while (!ring64_empty(ring)) ring64_push(new, ring64_pop(ring));
        free(ring);
        ring = new;
    }

    if (unlikely(ring->head == UINT16_MAX)) {
        ring->tail %= ring->cap;
        ring->head %= ring->cap;
    }

    ring->vals[ring->head % ring->cap] = val;
    ring->head++;
    return ring;
}
