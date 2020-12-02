/* bits.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// bits
// -----------------------------------------------------------------------------

inline size_t u32_clz(uint32_t x) { return x ? __builtin_clz(x) : 32; }

inline size_t u64_clz(uint64_t x) { return x ? __builtin_clzl(x) : 64; }
inline size_t u64_log2(uint64_t x) { return 63 - u64_clz(x); }


// -----------------------------------------------------------------------------
// math
// -----------------------------------------------------------------------------

inline uint32_t u32_min(uint32_t x, uint32_t y)
{
    return x <= y ? x : y;
}

inline int64_t i64_min(int64_t x, int64_t y)
{
    return x <= y ? x : y;
}

inline int64_t i64_max(int64_t x, int64_t y)
{
    return x >= y ? x : y;
}

inline int64_t i64_clamp(int64_t x, int64_t min, int64_t max)
{
    return i64_min(i64_max(x, min), max);
}

inline size_t align_cache(size_t sz)
{
    return sz ? ((((sz - 1) >> 6) + 1) << 6) : 0;
}
