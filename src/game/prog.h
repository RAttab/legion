/* program.h
   Rémi Attab (remi.attab@gmail.com), 02 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// prog
// -----------------------------------------------------------------------------

struct prog;

typedef uint8_t prog_it_t;
typedef uint8_t prog_id_t;

enum prog_state
{
    prog_eof = 0,
    prog_input,
    prog_output,
};

struct prog_ret
{
    enum prog_state state;
    enum item item;
};

void prog_load();
const struct prog *prog_fetch(prog_id_t prog);

prog_id_t prog_id(const struct prog *);
size_t prog_len(const struct prog *);
enum item prog_host(const struct prog *);
struct prog_ret prog_at(const struct prog *, prog_it_t index);


// -----------------------------------------------------------------------------
// packed
// -----------------------------------------------------------------------------

typedef uint64_t prog_packed_t;

inline prog_packed_t prog_pack(prog_id_t id, prog_it_t it, const struct prog *ptr)
{
    assert(!(((uintptr_t) ptr) >> 48));
    return (((uint64_t) id) << 56) | (((uint64_t) it) << 48) | ((uintptr_t) ptr);
}

inline const struct prog *prog_packed_ptr(prog_packed_t packed)
{
    return (void *) (((1UL << 48) - 1) & packed);
}

inline prog_packed_t prog_packed_ptr_update(prog_packed_t packed, const struct prog *ptr)
{
    assert(!(((uintptr_t) ptr) >> 48));
    return ((uintptr_t) ptr) | (packed & (~((1UL << 48) - 1)));
}

inline prog_id_t prog_packed_id(prog_packed_t packed)
{
    return packed >> 56;
}

inline prog_id_t prog_packed_it(prog_packed_t packed)
{
    return (packed >> 48) & 0xFF;
}

inline prog_packed_t prog_packed_it_inc(prog_packed_t packed)
{
    return packed + (1UL << 48);
}

inline prog_packed_t prog_packed_it_zero(prog_packed_t packed)
{
    return packed & (~(0xFFUL << 48));
}
