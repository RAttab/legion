/* tape.h
   Rémi Attab (remi.attab@gmail.com), 02 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/item.h"


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
struct tape_ret tape_at(const struct tape *, tape_it_t index);

void tapes_populate(void);
const struct tape *tapes_get(enum item id);


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

inline enum item tape_packed_it(tape_packed_t packed)
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
