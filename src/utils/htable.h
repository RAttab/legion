/* htable.h
   Rémi Attab (remi.attab@gmail.com), 10 Mar 2016
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// htable
// -----------------------------------------------------------------------------

struct htable_bucket
{
    uint64_t key;
    uint64_t value;
};

struct htable
{
    size_t len;
    size_t cap;
    struct htable_bucket *table;
};

struct htable_ret
{
    bool ok;
    uint64_t value;
};


void htable_clear(struct htable *);
void htable_reset(struct htable *);
void htable_reserve(struct htable *, size_t items);
struct htable htable_clone(const struct htable *);

struct htable_ret htable_get(const struct htable *, uint64_t key);
struct htable_ret htable_put(struct htable *, uint64_t key, uint64_t value);
struct htable_ret htable_try_put(struct htable *, uint64_t key, uint64_t value);
struct htable_ret htable_xchg(struct htable *, uint64_t key, uint64_t value);
struct htable_ret htable_del(struct htable *, uint64_t key);

bool htable_eq(const struct htable *lhs, const struct htable *rhs);

const struct htable_bucket *htable_next(
        const struct htable *, const struct htable_bucket *bucket);
