/* vec.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// vec64
// -----------------------------------------------------------------------------

#define vecx_type uint64_t
#define vecx_name vec64
#include "utils/vecx.h"

inline bool vec64_eq(const struct vec64 *lhs, const struct vec64 *rhs)
{
    if (lhs->len != rhs->len) return false;
    for (size_t i = 0; i < lhs->len; ++i)
        if (lhs->vals[i] != rhs->vals[i]) return false;
    return true;
}
