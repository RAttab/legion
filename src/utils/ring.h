/* ring.h
   Rémi Attab (remi.attab@gmail.com), 01 Jun 2021
   FreeBSD-style copyright and disclaimer apply

   \todo Could be improved alot
*/

#pragma once

// -----------------------------------------------------------------------------
// misc
// -----------------------------------------------------------------------------

inline size_t ring_len(uint16_t head, uint16_t tail)
{
    if (head == tail) return 0;
    if (tail < head) return head - tail;
    return (head + 1) + (UINT16_MAX - tail);
}


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
