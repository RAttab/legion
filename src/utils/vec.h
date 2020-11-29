/* vec.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// vec64
// -----------------------------------------------------------------------------

struct vec64
{
    uint32_t len, cap;
    uint64_t vals[];
};

inline void vec64_free(struct vec64 *vec) { free(vec); }

inline struct vec64 *vec64_reserve(size_t size)
{
    struct vec64 *vec = calloc(1, sizeof(*vec) + size * sizeof(vec->vals[0]));
    vec->cap = size;
    return vec;
}

inline struct vec64 *vec64_append(struct vec64 *vec, uint64_t val)
{
    if (unlikely(!vec || vec->len == vec->cap)) {
        size_t size = vec ? 1 : vec->cap * 2;
        vec = realloc(vec, sizeof(*vec) + size * sizeof(vec->vals[0]));
    }
    vec->vals[vec->len++] = val;
    return vec;
}
