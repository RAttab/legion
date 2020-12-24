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
inline size_t vec64_len(struct vec64 *vec) { return vec ? vec->len : 0; }
inline size_t vec64_cap(struct vec64 *vec) { return vec ? vec->cap : 0; }

inline struct vec64 *vec64_reserve(size_t size)
{
    struct vec64 *vec = calloc(1, sizeof(*vec) + size * sizeof(vec->vals[0]));
    vec->cap = size;
    return vec;
}

inline struct vec64 *vec64_append(struct vec64 *vec, uint64_t val)
{
    if (unlikely(!vec)) vec = vec64_reserve(1);
    if (unlikely(vec->len == vec->cap)) {
        size_t cap = vec->cap * 2;
        vec = realloc(vec, sizeof(*vec) + cap * sizeof(vec->vals[0]));
        vec->cap = cap;
    }
    vec->vals[vec->len++] = val;
    return vec;
}

inline struct vec64 *vec64_sort(struct vec64 *vec)
{
    // gcc extension
    int cmp(const void *l, const void *r) {
        uint64_t lhs = *((uint64_t *) l);
        uint64_t rhs = *((uint64_t *) r);
        return lhs == rhs ? 0 : (lhs < rhs ? -1 : 1);
    }
    qsort(vec->vals, vec->len, sizeof(vec->vals[0]), cmp);
}
