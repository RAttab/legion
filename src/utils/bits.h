/* bits.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply

   PCG: A Family of Simple Fast Space-Efficient Statistically Good Algorithms for Random Number Generation
     Author: Melissa E. O'Neill
     Institution: Harvey Mudd College
     Address: Claremont, CA
     Number: HMC-CS-2014-0905
     Year: 2014
     Month: Sep
     Xurl: https://www.cs.hmc.edu/tr/hmc-cs-2014-0905.pdf
     Implementation:
       Repo: https://github.com/imneme/pcg-c
       License: MIT-style license
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
// hash
// -----------------------------------------------------------------------------

// FNV-1a hash implementation: http://isthe.com/chongo/tech/comp/fnv/
inline uint64_t hash_str(const char *key, size_t len)
{
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < len; ++i)
        hash = (hash ^ key[i]) * 0x100000001b3;

    return hash;
}

// Reproduction of PCG-RXS-M-XS taken from pcg-c. All credits goes to the
// original author (see header).
inline uint64_t hash_u64(uint64_t value)
{
    // ref: pcg-c/include/pcg_variants.h
    //   math: pcg_oneseq_64_step_r
    //   const: PCG_DEFAULT_MULTIPLIER_64, PCG_DEFAULT_INCREMENT_64
    value = value * 6364136223846793005ULL + 1442695040888963407ULL;

    // ref: pcg-c/include/pcg_variants.h
    //   math: pcg_output_rxs_m_xs_64_64
    //   const: nil
    value = ((value >> ((value >> 59U) + 5U)) ^ value) * 12605985483714917081ULL;
    return (value >> 43u) ^ value;
}


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
