/* ring.h
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

// Important that ring_cap be much smaller then ring_it_t so that we can
// properly set an ordering between ring_it_t values by comparing their deltas
// against ring_cap. Important for computing save/load deltas.
typedef uint32_t ring_it_t;
enum { ring_cap = UINT16_MAX };

inline size_t ring_delta(ring_it_t first, ring_it_t last)
{
    if (first == last) return 0;
    if (first < last) return last - first;
    return (last + 1) + (UINT32_MAX - first);
}


// -----------------------------------------------------------------------------
// ack
// -----------------------------------------------------------------------------

struct ring_ack { ring_it_t head, tail; };

inline uint64_t ring_ack_to_u64(struct ring_ack ack)
{
    return (((uint64_t) ack.tail) << 32) | ack.head;
}

inline struct ring_ack ring_ack_from_u64(uint64_t ack)
{
    return (struct ring_ack) {
        .tail = ack >> 32,
        .head = ack & ((uint32_t) -1),
    };
}


// -----------------------------------------------------------------------------
// ring16
// -----------------------------------------------------------------------------

#define ringx_type uint16_t
#define ringx_name ring16
#define ringx_save
#include "utils/ringx.h"


// -----------------------------------------------------------------------------
// ring32
// -----------------------------------------------------------------------------

#define ringx_type uint32_t
#define ringx_name ring32
#define ringx_save
#include "utils/ringx.h"


// -----------------------------------------------------------------------------
// ring64
// -----------------------------------------------------------------------------

#define ringx_type uint64_t
#define ringx_name ring64
#define ringx_save
#include "utils/ringx.h"
