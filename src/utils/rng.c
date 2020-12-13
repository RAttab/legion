/* rng.c
   Rémi Attab (remi.attab@gmail.com), 25 Feb 2016
   FreeBSD-style copyright and disclaimer apply

   Xorshift random number generator for testing and statsd sampling

   See George Marsaglia (2003). Xorshift RNGs.  DOI: 10.18637/jss.v008.i14
     http://www.jstatsoft.org/article/view/v008i14
   (section 4, function xor128)

   Current implementation is the xorshift64* variant which has better
   statistical properties.

   TODO: Replace with PCG which is even better and allows for customization.
*/

#include "rng.h"


// -----------------------------------------------------------------------------
// init
// -----------------------------------------------------------------------------

struct rng rng_make(uint64_t seed)
{
    // We xor the seed with a randomly chosen number to avoid ending up with a 0
    // state which would be bad.
    struct rng rng = { .x = seed ^ UINT64_C(0xedef335f00e170b3) };
    assert(rng.x);
    return rng;
}



// -----------------------------------------------------------------------------
// gen
// -----------------------------------------------------------------------------

uint64_t rng_step(struct rng *rng)
{
    rng->x ^= rng->x >> 12;
    rng->x ^= rng->x << 25;
    rng->x ^= rng->x >> 27;
    return rng->x * UINT64_C(2685821657736338717);
}

bool rng_prob(struct rng *rng, double prob)
{
    return rng_step(rng) <= (uint64_t) (prob * rng_max());
}

uint64_t rng_uni(struct rng *rng, uint64_t min, uint64_t max)
{
    assert(max - min != 0);
    return rng_step(rng) % (max - min) + min;
}


uint64_t rng_exp(struct rng *rng, uint64_t min, uint64_t max)
{
    assert(max - min != 0);
    return rng_uni(rng, min, rng_uni(rng, min+1, max));
}

uint64_t rng_norm(struct rng *rng, uint64_t min, uint64_t max)
{
    return (rng_uni(rng, min, max) + rng_uni(rng, min, max)) / 2;
}
