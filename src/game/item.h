/* items.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
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
    ITEM_ELE_A = 0x01,
    ITEM_ELE_B,
    ITEM_ELE_C,
    ITEM_ELE_D,
    ITEM_ELE_E,
    ITEM_ELE_F,
    ITEM_ELE_G,
    ITEM_ELE_H,
    ITEM_ELE_I,
    ITEM_ELE_J,
    ITEM_ELE_K,
    ITEM_ELE_L,
    ITEM_ELE_M,
    ITEM_ELE_N,
    ITEM_ELE_O,
    ITEM_ELE_P,

    // Synth
    ITEM_ELE_Q,
    ITEM_ELE_R,
    ITEM_ELE_S,
    ITEM_ELE_T,
    ITEM_ELE_U,
    ITEM_ELE_V,
    ITEM_ELE_W,
    ITEM_ELE_X,
    ITEM_ELE_Y,
    ITEM_ELE_Z,

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
// id
// -----------------------------------------------------------------------------

typedef uint32_t id_t;

inline id_t make_id(item_t type, id_t id) { return type << 24 | id; }
inline item_t id_item(id_t id) { return id >> 24; }


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

enum { elements_len = ITEM_ELE_Z - ITEM_ELE_A };
typedef uint32_t elements_t [elements_len];
