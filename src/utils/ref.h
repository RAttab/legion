/* shared.h
   Rémi Attab (remi.attab@gmail.com), 30 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include <stdatomic.h>


// -----------------------------------------------------------------------------
// shared
// -----------------------------------------------------------------------------

// Must be the first argument in the object;
typedef atomic_size_t ref_t;

inline void ref_init(void *ptr)
{
    atomic_init((ref_t *) ptr, 1);
}

inline void *ref_share(void *ptr)
{
    size_t val = atomic_fetch_add((ref_t *) ptr, 1);
    assert(val);
    return ptr;
}

inline void ref_discard(void *ptr)
{
    size_t val = atomic_fetch_sub((ref_t *) ptr, 1);
    if (val == 1) free(ptr);
}
