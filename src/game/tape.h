/* tape.h
   Rémi Attab (remi.attab@gmail.com), 02 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/energy.h"
#include "db/items.h"
#include "utils/bits.h"


// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

typedef uint8_t im_work;


typedef uint8_t tape_it;

inline bool tape_it_validate(vm_word word)
{
    return word >= 0 && word <= UINT8_MAX;
}


enum legion_packed tape_state
{
    tape_eof = 0,
    tape_input,
    tape_work,
    tape_output,
};

inline bool tape_state_item(enum tape_state state)
{
    return state == tape_input || state == tape_output;
}


// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

struct tape
{
    enum item id;
    enum item host;
    im_work work;
    tape_it inputs, outputs;
    im_energy energy;
    enum item tape[];
};

struct tape_ret
{
    enum tape_state state;
    enum item item;
};

enum item tape_id(const struct tape *);
size_t tape_len(const struct tape *);
enum item tape_host(const struct tape *);
im_energy tape_energy(const struct tape *);
im_work tape_work_cap(const struct tape *);
struct tape_ret tape_at(const struct tape *, tape_it index);


// -----------------------------------------------------------------------------
// packed
// -----------------------------------------------------------------------------

typedef uint64_t tape_packed;

inline tape_packed tape_pack(enum item id, tape_it it, const struct tape *ptr)
{
    assert(!(((uintptr_t) ptr) >> 48));
    return (((uint64_t) id) << 56) | (((uint64_t) it) << 48) | ((uintptr_t) ptr);
}

inline const struct tape *tape_packed_ptr(tape_packed packed)
{
    return (void *) (((1UL << 48) - 1) & packed);
}

inline tape_packed tape_packed_ptr_update(tape_packed packed, const struct tape *ptr)
{
    assert(!(((uintptr_t) ptr) >> 48));
    return ((uintptr_t) ptr) | (packed & (~((1UL << 48) - 1)));
}

inline enum item tape_packed_id(tape_packed packed)
{
    return packed >> 56;
}

inline tape_it tape_packed_it(tape_packed packed)
{
    return (packed >> 48) & 0xFF;
}

inline tape_packed tape_packed_it_inc(tape_packed packed)
{
    return packed + (1UL << 48);
}

inline tape_packed tape_packed_it_zero(tape_packed packed)
{
    return packed & (~(0xFFUL << 48));
}


// -----------------------------------------------------------------------------
// tape_set
// -----------------------------------------------------------------------------

struct tape_set { uint64_t s[4]; };
static_assert(sizeof(struct tape_set) * 8 >= items_max);

size_t tape_set_len(const struct tape_set *);
bool tape_set_empty(const struct tape_set *);

bool tape_set_check(const struct tape_set *, enum item);
void tape_set_put(struct tape_set *, enum item);

struct tape_set tape_set_invert(struct tape_set *);
enum item tape_set_next(const struct tape_set *, enum item);

bool tape_set_eq(const struct tape_set *, const struct tape_set *);
void tape_set_union(struct tape_set *, const struct tape_set *);
size_t tape_set_intersect(const struct tape_set *, const struct tape_set *);

void tape_set_save(const struct tape_set *, struct save *);
bool tape_set_load(struct tape_set *, struct save *);
