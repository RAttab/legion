/* rng.c
   Rémi Attab (remi.attab@gmail.com), 25 Feb 2016
   FreeBSD-style copyright and disclaimer apply

   Extracted from PCG's PCG-RXS-M-XS (64 bit state, 64-bit output).

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

#include "rng.h"


// -----------------------------------------------------------------------------
// init
// -----------------------------------------------------------------------------

struct rng rng_make(uint64_t seed)
{
    return (struct rng) { .x = seed };
}


// -----------------------------------------------------------------------------
// gen
// -----------------------------------------------------------------------------

// Reproduction of PCG-RXS-M-XS taken from pcg-c. All credits goes to the
// original author (see header).
uint64_t rng_step(struct rng *rng)
{
    // ref: pcg-c/include/pcg_variants.h
    //   math: pcg_oneseq_64_step_r
    //   const: PCG_DEFAULT_MULTIPLIER_64, PCG_DEFAULT_INCREMENT_64
    rng->x = rng->x * 6364136223846793005ULL + 1442695040888963407ULL;

    // ref: pcg-c/include/pcg_variants.h
    //   math: pcg_output_rxs_m_xs_64_64
    //   const: nil
    uint64_t x = rng->x;
    x = ((x >> ((x >> 59U) + 5U)) ^ x) * 12605985483714917081ULL;
    return (x >> 43u) ^ x;
}

bool rng_prob(struct rng *rng, double prob)
{
    return rng_step(rng) <= (uint64_t) (prob * rng_max());
}

uint64_t rng_uni(struct rng *rng, uint64_t min, uint64_t max)
{
    assert(min < max);
    return rng_step(rng) % (max - min) + min;
}


uint64_t rng_exp(struct rng *rng, uint64_t min, uint64_t max)
{
    assert(max - min != 0);
    return rng_uni(rng, min, rng_uni(rng, min+1, max+1));
}

uint64_t rng_norm(struct rng *rng, uint64_t min, uint64_t max)
{
    return (rng_uni(rng, min, max) + rng_uni(rng, min, max)) / 2;
}
