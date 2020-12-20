/* items.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/str.h"
#include "utils/bits.h"


// -----------------------------------------------------------------------------
// item
// -----------------------------------------------------------------------------

// enum item represents the values while item_t represents the storage size. Use
// item_t when defining the type. The enum is mostly for the constants.
typedef uint8_t item_t;

enum item
{
    ITEM_NIL = 0x00,

    // Natural
    ITEM_ELEM_A = 0x01,
    ITEM_ELEM_B,
    ITEM_ELEM_C,
    ITEM_ELEM_D,
    ITEM_ELEM_E,
    ITEM_ELEM_F,
    ITEM_ELEM_G,
    ITEM_ELEM_H,
    ITEM_ELEM_I,
    ITEM_ELEM_J,
    ITEM_ELEM_K,
    ITEM_ELEM_L,
    ITEM_ELEM_M,
    ITEM_ELEM_N,
    ITEM_ELEM_O,
    ITEM_ELEM_P,

    // Synth
    ITEM_ELEM_Q,
    ITEM_ELEM_R,
    ITEM_ELEM_S,
    ITEM_ELEM_T,
    ITEM_ELEM_U,
    ITEM_ELEM_V,
    ITEM_ELEM_W,
    ITEM_ELEM_X,
    ITEM_ELEM_Y,
    ITEM_ELEM_Z,

    // ... moi j'connai mon alphabet ...

    // Active
    ITEM_BRAIN = 0x20,
    ITEM_WORKER,
    ITEM_PRINTER,
    ITEM_LAB,
    ITEM_COMM,
    ITEM_SHIP,

    // Placeholders for the other shit
    ITEM_OTHERS = 0x30,
};

// -----------------------------------------------------------------------------
// ele
// -----------------------------------------------------------------------------

enum ele
{
    elem_natural_first = ITEM_ELEM_A,
    elem_natural_last = ITEM_ELEM_P,
    elem_natural_len = (elem_natural_last + 1) - elem_natural_first,

    elem_synth_first = ITEM_ELEM_Q,
    elem_synth_last = ITEM_ELEM_Z,
    elem_synth_len = (elem_synth_last + 1) - elem_synth_first,
};

static_assert(elem_natural_len + elem_synth_len == 26);


// -----------------------------------------------------------------------------
// id
// -----------------------------------------------------------------------------

typedef uint32_t id_t;

inline id_t make_id(item_t type, id_t id) { return type << 24 | id; }
inline item_t id_item(id_t id) { return id >> 24; }
inline uint32_t id_bot(id_t id) { return id & ((1 << 24) - 1); }

enum { id_str_len = 2+6 };
inline void id_str(id_t id, size_t len, char *dst)
{
    assert(id);
    assert(len >= id_str_len);

    switch(id_item(id)) {
    case ITEM_BRAIN: { dst[0] = 'b'; break; }
    case ITEM_WORKER: { dst[0] = 'w'; break; }
    case ITEM_PRINTER: { dst[0] = 'p'; break; }
    case ITEM_LAB: { dst[0] = 'l'; break; }
    case ITEM_COMM: { dst[0] = 'c'; break; }
    case ITEM_SHIP: { dst[0] = 's'; break; }
    default: { assert(false && "unsuported item in id_str"); }
    }

    id = id_bot(id);
    assert(id);

    for (size_t i = 0; i < 6; i++, id >>=4)
        dst[6-i] = str_hexchar(id);
}


// -----------------------------------------------------------------------------
// cargo
// -----------------------------------------------------------------------------

typedef uint8_t item_t;
typedef uint16_t cargo_t;

inline cargo_t make_cargo(item_t item, uint8_t count)
{
    return (((cargo_t) item) << 8) | count;
}

inline item_t cargo_item(cargo_t cargo) { return cargo >> 8; }
inline uint8_t cargo_count(cargo_t cargo) { return cargo; }

inline cargo_t cargo_add(cargo_t cargo, uint8_t val)
{
    size_t count = cargo_count(cargo);
    return (cargo & ~0xFF) | i64_min(0xFF, count + val);
}

inline cargo_t cargo_sub(cargo_t cargo, uint8_t val)
{
    ssize_t count = cargo_count(cargo);
    return (cargo & ~0xFF) | i64_max(0, count - val);
}


// -----------------------------------------------------------------------------
// elements
// -----------------------------------------------------------------------------

enum { elements_len = 26 };
typedef uint32_t elements_t [elements_len];
