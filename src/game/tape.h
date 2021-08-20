/* tape.h
   Rémi Attab (remi.attab@gmail.com), 02 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "items/item.h"
#include "utils/bits.h"


// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

struct tape;

typedef uint8_t tape_it_t;

enum tape_state
{
    tape_eof = 0,
    tape_input,
    tape_output,
};

struct tape_ret
{
    enum tape_state state;
    enum item item;
};

enum item tape_id(const struct tape *);
size_t tape_len(const struct tape *);
enum item tape_host(const struct tape *);
uint8_t tape_bits(const struct tape *);
struct tape_ret tape_at(const struct tape *, tape_it_t index);


// -----------------------------------------------------------------------------
// packed
// -----------------------------------------------------------------------------

typedef uint64_t tape_packed_t;

inline tape_packed_t tape_pack(enum item id, tape_it_t it, const struct tape *ptr)
{
    assert(!(((uintptr_t) ptr) >> 48));
    return (((uint64_t) id) << 56) | (((uint64_t) it) << 48) | ((uintptr_t) ptr);
}

inline const struct tape *tape_packed_ptr(tape_packed_t packed)
{
    return (void *) (((1UL << 48) - 1) & packed);
}

inline tape_packed_t tape_packed_ptr_update(tape_packed_t packed, const struct tape *ptr)
{
    assert(!(((uintptr_t) ptr) >> 48));
    return ((uintptr_t) ptr) | (packed & (~((1UL << 48) - 1)));
}

inline enum item tape_packed_id(tape_packed_t packed)
{
    return packed >> 56;
}

inline tape_it_t tape_packed_it(tape_packed_t packed)
{
    return (packed >> 48) & 0xFF;
}

inline tape_packed_t tape_packed_it_inc(tape_packed_t packed)
{
    return packed + (1UL << 48);
}

inline tape_packed_t tape_packed_it_zero(tape_packed_t packed)
{
    return packed & (~(0xFFUL << 48));
}


// -----------------------------------------------------------------------------
// tape_set
// -----------------------------------------------------------------------------

struct tape_set { uint64_t s[4]; };
static_assert(sizeof(struct tape_set) * 8 >= ITEM_MAX);

inline bool tape_set_empty(const struct tape_set *set)
{
    for (size_t i = 0; i < array_len(set->s); ++i)
        if (set->s[i]) return false;
    return true;
}

inline bool tape_set_check(const struct tape_set *set, enum item item)
{
    return set->s[item / 64] & (1ULL << (item % 64));
}

inline void tape_set_put(struct tape_set *set, enum item item)
{
    set->s[item / 64] |= 1ULL << (item % 64);
}

inline enum item tape_set_next(const struct tape_set *set, enum item first)
{
    uint64_t mask = (1ULL << ((first+1) % 64)) - 1;
    for (size_t i = first / 64; i < array_len(set->s); ++i) {
        uint64_t x = set->s[i] & ~mask;
        if (x) return u64_ctz(x) + i * 64;
        mask = 0;
    }
    return ITEM_NIL;
}

inline void tape_set_union(struct tape_set *set, const struct tape_set *other)
{
    for (size_t i = 0; i < array_len(set->s); ++i)
        set->s[i] |= other->s[i];
}

inline void tape_set_intersect(struct tape_set *set, const struct tape_set *other)
{
    for (size_t i = 0; i < array_len(set->s); ++i)
        set->s[i] &= other->s[i];
}


// -----------------------------------------------------------------------------
// tapes
// -----------------------------------------------------------------------------

void tapes_populate(void);
const struct tape *tapes_get(enum item id);

struct tape_stats
{
    size_t rank;
    struct tape_set reqs;
    uint16_t elems[ITEMS_NATURAL_LEN];
};
const struct tape_stats *tapes_stats(enum item id);
