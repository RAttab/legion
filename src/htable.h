/* htable.h
   Rémi Attab (remi.attab@gmail.com), 10 Mar 2016
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


// -----------------------------------------------------------------------------
// struct
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


void htable_reset(struct htable *);
void htable_reserve(struct htable *, size_t items);
struct htable_ret htable_get(struct htable *, uint64_t key);
struct htable_ret htable_put(struct htable *, uint64_t key, uint64_t value);
