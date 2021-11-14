/* vecx.h
   Rémi Attab (remi.attab@gmail.com), 02 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#ifndef vecx_type
# error "vecx_type must be declared when including vecx.h"
#endif

#ifndef vecx_name
# error "vecx_name must be declared when including vecx.h"
#endif

#define vecx_fn_concat(prefix, suffix) prefix ## _ ## suffix
#define vecx_fn_eval(prefix, suffix) vecx_fn_concat(prefix, suffix)
#define vecx_fn(name) vecx_fn_eval(vecx_name, name)


#include "common.h"


// -----------------------------------------------------------------------------
// vecx
// -----------------------------------------------------------------------------

struct vecx_name
{
    uint32_t len, cap;
    vecx_type vals[];
};

inline void vecx_fn(free) (struct vecx_name *vec) { free(vec); }
inline size_t vecx_fn(len) (struct vecx_name *vec) { return vec ? vec->len : 0; }
inline size_t vecx_fn(cap) (struct vecx_name *vec) { return vec ? vec->cap : 0; }

inline struct vecx_name *vecx_fn(reserve) (size_t size)
{
    struct vecx_name *vec = calloc(1, sizeof(*vec) + size * sizeof(vec->vals[0]));
    vec->cap = size;
    return vec;
}

inline struct vecx_name *vecx_fn(grow) (struct vecx_name *vec, size_t size)
{
    if (!vec) return vecx_fn(reserve)(size);
    if (size <= vec->cap) return vec;

    vec = realloc(vec, sizeof(*vec) + size * sizeof(vec->vals[0]));
    vec->cap = size;
    return vec;
}

inline struct vecx_name *vecx_fn(append) (struct vecx_name *vec, vecx_type val)
{
    if (unlikely(!vec)) vec = vecx_fn(reserve)(1);
    if (unlikely(vec->len == vec->cap)) vec = vecx_fn(grow)(vec, vec->cap * 2);
    vec->vals[vec->len] = val;
    vec->len++;
    return vec;
}

inline struct vecx_name *vecx_fn(copy) (struct vecx_name *old)
{
    struct vecx_name *new = vecx_fn(reserve)(old->cap);
    memcpy(new, old, sizeof(*old) + old->len * sizeof(old->vals[0]));
    return new;
}

#ifdef vecx_sort
inline void vecx_fn(sort) (struct vecx_name *vec)
{
    // gcc extension
    int cmp(const void *l, const void *r) {
        vecx_type lhs = *((vecx_type *) l);
        vecx_type rhs = *((vecx_type *) r);
        return lhs == rhs ? 0 : (lhs < rhs ? -1 : 1);
    }
    qsort(vec->vals, vec->len, sizeof(vec->vals[0]), cmp);
}

inline void vecx_fn(sort_fn) (
        struct vecx_name *vec,
        int (*fn) (const void *, const void *))
{
    qsort(vec->vals, vec->len, sizeof(vec->vals[0]), fn);
}
#endif

#undef vecx_fn_concat
#undef vecx_fn_eval
#undef vecx_fn

#undef vecx_sort
#undef vecx_type
#undef vecx_name
