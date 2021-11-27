/* bits.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// bits
// -----------------------------------------------------------------------------

inline size_t u64_pop(uint64_t x) { return __builtin_popcountl(x); }

inline size_t u32_clz(uint32_t x) { return likely(x) ? __builtin_clz(x) : 32; }
inline size_t u64_clz(uint64_t x) { return likely(x) ? __builtin_clzl(x) : 64; }
inline size_t u64_log2(uint64_t x) { return likely(x) ? 63 - u64_clz(x) : 0; }

inline size_t u64_ctz(uint64_t x) { return likely(x) ? __builtin_ctzl(x) : 0; }


// -----------------------------------------------------------------------------
// math
// -----------------------------------------------------------------------------

inline uint32_t u64_top(uint64_t x) { return x >> 32; }
inline uint32_t u64_bot(uint64_t x) { return x << 32 >> 32; }

inline int64_t i64_ceil_div(int64_t x, int64_t d)
{

    return (d && x) ? (x - 1) / d + 1 : 1;
}

inline int64_t u64_ceil_div(uint64_t x, uint64_t d)
{
    return (d && x) ? (x - 1) / d + 1 : 1;
}

inline int64_t i64_clamp(int64_t x, int64_t min, int64_t max)
{
    return legion_min(legion_max(x, min), max);
}

inline size_t align_cache(size_t sz)
{
    return sz ? ((((sz - 1) >> 6) + 1) << 6) : 0;
}

inline uint16_t u16_saturate_add(uint64_t val, uint64_t add)
{
    val += add;
    return val > UINT16_MAX ? UINT16_MAX : val;
}

inline uint32_t u32_saturate_add(uint64_t val, uint64_t add)
{
    val += add;
    return val > UINT32_MAX ? UINT32_MAX : val;
}
