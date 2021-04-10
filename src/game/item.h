/* item.h
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
    ITEM_ELEM_B = 0x02,
    ITEM_ELEM_C = 0x03,
    ITEM_ELEM_D = 0x04,
    ITEM_ELEM_E = 0x05,
    ITEM_ELEM_F = 0x06,
    ITEM_ELEM_G = 0x07,
    ITEM_ELEM_H = 0x08,
    ITEM_ELEM_I = 0x09,
    ITEM_ELEM_J = 0x0A,
    ITEM_ELEM_K = 0x0B,
    ITEM_ELEM_L = 0x0C,
    ITEM_ELEM_M = 0x0D,
    ITEM_ELEM_N = 0x0E,
    ITEM_ELEM_O = 0x0F,
    ITEM_ELEM_P = 0x10,

    // Synth
    ITEM_ELEM_Q = 0x11,
    ITEM_ELEM_R = 0x12,
    ITEM_ELEM_S = 0x13,
    ITEM_ELEM_T = 0x14,
    ITEM_ELEM_U = 0x15,
    ITEM_ELEM_V = 0x16,
    ITEM_ELEM_W = 0x17,
    ITEM_ELEM_X = 0x18,
    ITEM_ELEM_Y = 0x19,
    ITEM_ELEM_Z = 0x1A,

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

    ITEM_MAX,
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


// -----------------------------------------------------------------------------
// schema
// -----------------------------------------------------------------------------

enum { schema_len = 3*3*3 };
typedef item_t schema_t[schema_len];

inline item_t *schema_idx(schema_t schema, size_t x, size_t y, size_t z)
{
    return &schema[x + y*3 + z*3*3];
}

inline uint64_t schema_hash(const schema_t schema)
{
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < schema_len; ++i)
        hash = (hash ^ schema[i]) * 0x100000001b3;
    return hash;
}

