/* rng.h
   Rémi Attab (remi.attab@gmail.com), 25 Feb 2016
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// rng
// -----------------------------------------------------------------------------

struct rng { uint64_t x; };
struct rng rng_make(uint64_t seed);

inline uint64_t rng_max() { return (uint64_t) -1UL; }


// -----------------------------------------------------------------------------
// gen
// -----------------------------------------------------------------------------

uint64_t rng_step(struct rng *rng);
bool rng_prob(struct rng *rng, double prob);

// min: inclusive, max: exclusive
uint64_t rng_uni(struct rng *rng, uint64_t min, uint64_t max);
uint64_t rng_exp(struct rng *rng, uint64_t min, uint64_t max);
uint64_t rng_norm(struct rng *rng, uint64_t min, uint64_t max);

