/* item.h
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"

// -----------------------------------------------------------------------------
// item
// -----------------------------------------------------------------------------

enum legion_packed item
{
    ITEM_NIL = 0x00,


    // -------------------------------------------------------------------------
    // Elements
    // -------------------------------------------------------------------------

    ITEM_NATURAL_FIRST = 0x01,
    // extract
    ITEM_ELEM_A = 0x01,
    ITEM_ELEM_B = 0x02,
    ITEM_ELEM_C = 0x03,
    ITEM_ELEM_D = 0x04,
    ITEM_ELEM_E = 0x05,
    ITEM_ELEM_F = 0x06,
    // condenser
    ITEM_ELEM_G = 0x07,
    ITEM_ELEM_H = 0x08,
    ITEM_ELEM_I = 0x09,
    ITEM_ELEM_J = 0x0A,
    // siphon
    ITEM_ELEM_K = 0x0B,
    ITEM_NATURAL_LAST,

    ITEM_SYNTH_FIRST = 0x0C,
    // collider
    ITEM_ELEM_L = 0x0C,
    ITEM_ELEM_M = 0x0D,
    ITEM_ELEM_N = 0x0E,
    ITEM_ELEM_O = 0x0F,
    ITEM_ELEM_P = 0x10,
    ITEM_ELEM_Q = 0x11,
    // collider + k
    ITEM_ELEM_R = 0x12,
    ITEM_ELEM_S = 0x13,
    ITEM_ELEM_T = 0x14,
    ITEM_ELEM_U = 0x15,
    ITEM_ELEM_V = 0x16,
    // meld
    ITEM_ELEM_W = 0x17,
    ITEM_ELEM_X = 0x18,
    ITEM_ELEM_Y = 0x19,
    // blackhole
    ITEM_ELEM_Z = 0x1A,
    ITEM_SYNTH_LAST,


    // -------------------------------------------------------------------------
    // Passive
    // -------------------------------------------------------------------------

    ITEM_PASSIVE_FIRST = 0x20,

    // T0 - printer
    ITEM_FRAME        = ITEM_PASSIVE_FIRST + 0x01,
    ITEM_LOGIC        = ITEM_PASSIVE_FIRST + 0x02,
    ITEM_GEAR         = ITEM_PASSIVE_FIRST + 0x03,
    ITEM_NEURON       = ITEM_PASSIVE_FIRST + 0x04,
    ITEM_BOND         = ITEM_PASSIVE_FIRST + 0x05,
    ITEM_MAGNET       = ITEM_PASSIVE_FIRST + 0x06,
    ITEM_NUCLEAR      = ITEM_PASSIVE_FIRST + 0x07,
    // T0 - assembly
    ITEM_ROBOTICS     = ITEM_PASSIVE_FIRST + 0x08,
    ITEM_CORE         = ITEM_PASSIVE_FIRST + 0x09,
    ITEM_CAPACITOR    = ITEM_PASSIVE_FIRST + 0x0A,
    ITEM_MATRIX       = ITEM_PASSIVE_FIRST + 0x0B,
    ITEM_MAGNET_FIELD = ITEM_PASSIVE_FIRST + 0x0C,
    ITEM_HULL         = ITEM_PASSIVE_FIRST + 0x0D,

    // T1
    ITEM_CAPTOR       = ITEM_PASSIVE_FIRST + 0x10,
    ITEM_LIQUID_FRAME = ITEM_PASSIVE_FIRST + 0x11,
    ITEM_RADIATION    = ITEM_PASSIVE_FIRST + 0x12,
    ITEM_ANTENNA      = ITEM_PASSIVE_FIRST + 0x13,
    ITEM_ACCELERATOR  = ITEM_PASSIVE_FIRST + 0x14,

    ITEM_PASSIVE_LAST,


    // -------------------------------------------------------------------------
    // Active
    // -------------------------------------------------------------------------

    ITEM_ACTIVE_FIRST = 0x80,

    // T0
    ITEM_DEPLOY   = ITEM_ACTIVE_FIRST + 0x00,
    ITEM_EXTRACT  = ITEM_ACTIVE_FIRST + 0x01,
    ITEM_PRINTER  = ITEM_ACTIVE_FIRST + 0x02,
    ITEM_ASSEMBLY = ITEM_ACTIVE_FIRST + 0x03,
    ITEM_MEMORY   = ITEM_ACTIVE_FIRST + 0x04,
    ITEM_BRAIN    = ITEM_ACTIVE_FIRST + 0x05,
    ITEM_SCANNER  = ITEM_ACTIVE_FIRST + 0x06,
    ITEM_LEGION   = ITEM_ACTIVE_FIRST + 0x07,
    ITEM_LAB      = ITEM_ACTIVE_FIRST + 0x08,
    ITEM_TEST     = ITEM_ACTIVE_FIRST + 0x0F, // Used in tests only.

    // T1
    ITEM_STORAGE      = ITEM_ACTIVE_FIRST + 0x10,
    ITEM_PORT         = ITEM_ACTIVE_FIRST + 0x11,
    ITEM_CONDENSER    = ITEM_ACTIVE_FIRST + 0x12,
    ITEM_AUTO_DEPLOY  = ITEM_ACTIVE_FIRST + 0x13,
    ITEM_TRANSMIT     = ITEM_ACTIVE_FIRST + 0x14,
    ITEM_RECEIVE      = ITEM_ACTIVE_FIRST + 0x15,
    ITEM_COLLIDER     = ITEM_ACTIVE_FIRST + 0x16,

    ITEM_ACTIVE_LAST,


    // -------------------------------------------------------------------------
    // Logistics
    // -------------------------------------------------------------------------

    ITEM_LOGISTICS_FIRST = 0xE0,
    ITEM_WORKER          = ITEM_LOGISTICS_FIRST + 0x00,
    ITEM_BULLET          = ITEM_LOGISTICS_FIRST + 0x01,
    ITEM_SOLAR           = ITEM_LOGISTICS_FIRST + 0x08,
    ITEM_KWHEEL          = ITEM_LOGISTICS_FIRST + 0x09,
    ITEM_ENERGY_STORE    = ITEM_LOGISTICS_FIRST + 0x0F,
    ITEM_LOGISTICS_LAST,

    // -------------------------------------------------------------------------
    // SYS
    // -------------------------------------------------------------------------

    ITEM_SYS_FIRST = 0xF0,
    ITEM_DATA   = ITEM_SYS_FIRST + 0x00,
    ITEM_ENERGY = ITEM_SYS_FIRST + 0x01,
    ITEM_SYS_LAST,


    ITEM_MAX = ITEM_SYS_LAST,
};

static_assert(sizeof(enum item) == 1);
static_assert(ITEM_NATURAL_LAST <= ITEM_SYNTH_FIRST);
static_assert(ITEM_SYNTH_LAST   <= ITEM_PASSIVE_FIRST);
static_assert(ITEM_PASSIVE_LAST <= ITEM_ACTIVE_FIRST);
static_assert(ITEM_ACTIVE_LAST  <= ITEM_LOGISTICS_FIRST);


// -----------------------------------------------------------------------------
// Categories
// -----------------------------------------------------------------------------

typedef const enum item *im_list_t;
extern im_list_t im_list_control;
extern im_list_t im_list_factory;

enum
{
    ITEMS_NATURAL_LEN   = ITEM_NATURAL_LAST   - ITEM_NATURAL_FIRST,
    ITEMS_SYNTH_LEN     = ITEM_SYNTH_LAST     - ITEM_SYNTH_FIRST,
    ITEMS_PASSIVE_LEN   = ITEM_PASSIVE_LAST   - ITEM_PASSIVE_FIRST,
    ITEMS_ACTIVE_LEN    = ITEM_ACTIVE_LAST    - ITEM_ACTIVE_FIRST,
    ITEMS_LOGISTICS_LEN = ITEM_LOGISTICS_LAST - ITEM_LOGISTICS_FIRST,
};

static_assert(ITEMS_NATURAL_LEN + ITEMS_SYNTH_LEN == 26);

inline bool item_is_active(enum item item)
{
    return item >= ITEM_ACTIVE_FIRST && item < ITEM_ACTIVE_LAST;
}

inline bool item_is_logistics(enum item item)
{
    return item >= ITEM_LOGISTICS_FIRST && item < ITEM_LOGISTICS_LAST;
}


// -----------------------------------------------------------------------------
// String
// -----------------------------------------------------------------------------

enum { item_str_len = 16 };

size_t item_str(enum item item, char *dst, size_t len);
const char *item_str_c(enum item item);
