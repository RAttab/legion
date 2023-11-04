/* hset.h
   Rémi Attab (remi.attab@gmail.com), 17 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// hset
// -----------------------------------------------------------------------------

struct hset
{
    size_t len, cap;
    uint64_t set[];
};

struct hset *hset_reserve(size_t len);
void hset_clear(struct hset *);
void hset_free(struct hset *);
struct hset *hset_clone(const struct hset *);

#define make_hset(...)                                          \
    ({                                                          \
        const uint64_t values[] = { __VA_ARGS__ };              \
        struct hset *hs = hset_reserve(array_len(values));      \
        for (size_t i = 0; i < array_len(values); ++i)          \
            hs = hset_put(hs, values[i]);                       \
        hs;                                                     \
    })

bool hset_test(const struct hset *, uint64_t key);
struct hset *hset_put(struct hset *, uint64_t key);
bool hset_del(struct hset *, uint64_t key);

typedef const uint64_t *hset_it;
hset_it hset_next(const struct hset *, hset_it);

bool hset_eq(const struct hset *, const struct hset *);
struct hset *hset_diff(const struct hset *, const struct hset *);
struct hset *hset_union(const struct hset *, const struct hset *);
struct hset *hset_intersect(const struct hset *, const struct hset *);

size_t hset_str(const struct hset *, char *str, size_t len);
