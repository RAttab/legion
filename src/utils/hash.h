/* hash.h
   Rémi Attab (remi.attab@gmail.com), 27 Nov 2021
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
// type
// -----------------------------------------------------------------------------

typedef uint64_t hash_t;


// -----------------------------------------------------------------------------
// hash pcg
// -----------------------------------------------------------------------------

// Reproduction of PCG-RXS-M-XS taken from pcg-c. All credits goes to the
// original author (see header).
inline hash_t hash_u64(uint64_t value)
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
// hash fnv
// -----------------------------------------------------------------------------
// FNV-1a hash implementation: http://isthe.com/chongo/tech/comp/fnv/

inline hash_t hash_init(void) { return 0xCBF29CE484222325ULL; }

inline hash_t hash_bytes(hash_t hash, const void *raw, size_t len)
{
    const uint8_t *bytes = raw;
    for (size_t i = 0; i < len; ++i)
        hash = (hash ^ bytes[i]) * 0x100000001B3;
    return hash;
}


inline hash_t hash_str(const char *str, size_t len)
{
    return hash_bytes(hash_init(), str, len);
}

#define hash_value(hash, value)                         \
    ({                                                  \
        hash_t __hash = (hash);                         \
        typeof(value) __value = (value);                \
        hash_bytes(__hash, &__value, sizeof(__value));   \
    })
