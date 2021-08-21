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
    ITEM_NATURAL_LAST,

    ITEM_SYNTH_FIRST = 0x0C,
    ITEM_ELEM_L = 0x0C,
    ITEM_ELEM_M = 0x0D,
    ITEM_ELEM_N = 0x0E,
    ITEM_ELEM_O = 0x0F,
    ITEM_ELEM_P = 0x10,
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
    ITEM_SYNTH_LAST,


    // -------------------------------------------------------------------------
    // Passive
    // -------------------------------------------------------------------------

    ITEM_PASSIVE_FIRST = 0x20,

    ITEM_FRAME      = ITEM_PASSIVE_FIRST + 0x01,
    ITEM_GEAR       = ITEM_PASSIVE_FIRST + 0x02,
    ITEM_FUEL       = ITEM_PASSIVE_FIRST + 0x03,
    ITEM_BONDING    = ITEM_PASSIVE_FIRST + 0x04,
    ITEM_CIRCUIT    = ITEM_PASSIVE_FIRST + 0x06,
    ITEM_NEURAL     = ITEM_PASSIVE_FIRST + 0x07,
    ITEM_SERVO      = ITEM_PASSIVE_FIRST + 0x10,
    ITEM_THRUSTER   = ITEM_PASSIVE_FIRST + 0x11,
    ITEM_PROPULSION = ITEM_PASSIVE_FIRST + 0x12,
    ITEM_PLATE      = ITEM_PASSIVE_FIRST + 0x13,
    ITEM_SHIELDING  = ITEM_PASSIVE_FIRST + 0x14,
    ITEM_HULL_1     = ITEM_PASSIVE_FIRST + 0x1A,
    ITEM_CORE       = ITEM_PASSIVE_FIRST + 0x20,
    ITEM_MATRIX     = ITEM_PASSIVE_FIRST + 0x21,
    ITEM_DATABANK   = ITEM_PASSIVE_FIRST + 0x22,

    ITEM_PASSIVE_LAST,


    // -------------------------------------------------------------------------
    // Active
    // -------------------------------------------------------------------------

    ITEM_ACTIVE_FIRST = 0x80,

    ITEM_DEPLOY     = ITEM_ACTIVE_FIRST + 0x00,
    ITEM_EXTRACT_1  = ITEM_ACTIVE_FIRST + 0x01,
    ITEM_EXTRACT_2  = ITEM_ACTIVE_FIRST + 0x02,
    ITEM_EXTRACT_3  = ITEM_ACTIVE_FIRST + 0x03,
    ITEM_PRINTER_1  = ITEM_ACTIVE_FIRST + 0x04,
    ITEM_PRINTER_2  = ITEM_ACTIVE_FIRST + 0x05,
    ITEM_PRINTER_3  = ITEM_ACTIVE_FIRST + 0x06,
    ITEM_ASSEMBLY_1 = ITEM_ACTIVE_FIRST + 0x07,
    ITEM_ASSEMBLY_2 = ITEM_ACTIVE_FIRST + 0x08,
    ITEM_ASSEMBLY_3 = ITEM_ACTIVE_FIRST + 0x09,
    ITEM_STORAGE    = ITEM_ACTIVE_FIRST + 0x0A,
    ITEM_SCANNER_1  = ITEM_ACTIVE_FIRST + 0x0B,
    ITEM_SCANNER_2  = ITEM_ACTIVE_FIRST + 0x0C,
    ITEM_SCANNER_3  = ITEM_ACTIVE_FIRST + 0x0D,
    ITEM_LAB        = ITEM_ACTIVE_FIRST + 0x0F,
    ITEM_DB_1       = ITEM_ACTIVE_FIRST + 0x10,
    ITEM_DB_2       = ITEM_ACTIVE_FIRST + 0x11,
    ITEM_DB_3       = ITEM_ACTIVE_FIRST + 0x12,
    ITEM_BRAIN_1    = ITEM_ACTIVE_FIRST + 0x13,
    ITEM_BRAIN_2    = ITEM_ACTIVE_FIRST + 0x14,
    ITEM_BRAIN_3    = ITEM_ACTIVE_FIRST + 0x15,
    ITEM_LEGION_1   = ITEM_ACTIVE_FIRST + 0x16,
    ITEM_LEGION_2   = ITEM_ACTIVE_FIRST + 0x17,
    ITEM_LEGION_3   = ITEM_ACTIVE_FIRST + 0x18,

    ITEM_ACTIVE_LAST,


    // -------------------------------------------------------------------------
    // Logistics
    // -------------------------------------------------------------------------

    ITEM_LOGISTICS_FIRST = 0xF0,
    ITEM_WORKER          = ITEM_LOGISTICS_FIRST + 0x00,
    ITEM_BULLET          = ITEM_LOGISTICS_FIRST + 0x01,
    ITEM_LOGISTICS_LAST,


    ITEM_MAX = ITEM_LOGISTICS_LAST,
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
extern im_list_t im_list_t0;

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
