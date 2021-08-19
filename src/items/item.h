/* item.h
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// item
// -----------------------------------------------------------------------------

enum item
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
    ITEM_ELEM_LAST,


    // -------------------------------------------------------------------------
    // Passive
    // -------------------------------------------------------------------------

    ITEM_PASSIVE_FIRST = 0x20,

    ITEM_FRAME         = 0x20,
    ITEM_LOGIC         = 0x21,
    ITEM_NEURON        = 0x22,
    ITEM_BOND          = 0x23,
    ITEM_MAGNET        = 0x24,
    ITEM_NUCLEAR       = 0x25,
    ITEM_ROBOTICS      = 0x26,
    ITEM_CORE          = 0x27,
    ITEM_CAPACITOR     = 0x28,
    ITEM_MATRIX        = 0x29,
    ITEM_MAGNET_FIELD  = 0x2A,
    ITEM_HULL          = 0x2B,

    ITEM_PASSIVE_LAST,


    // -------------------------------------------------------------------------
    // Active
    // -------------------------------------------------------------------------

    ITEM_ACTIVE_FIRST = 0x80,

    ITEM_DEPLOY   = 0x80,
    ITEM_EXTRACT  = 0x81,
    ITEM_PRINTER  = 0x82,
    ITEM_ASSEMBLY = 0x83,
    ITEM_BRAIN    = 0x84,
    ITEM_MEMORY   = 0x85,
    ITEM_SCANNER  = 0x86,
    ITEM_LEGION   = 0x87,
    ITEM_LAB      = 0x88,

    ITEM_CODENSER    = 0x90,
    ITEM_COLIDER     = 0x91,
    ITEM_PORT        = 0x92,
    ITEM_SOLAR       = 0x93,
    ITEM_CAPACITOR   = 0x94,
    ITEM_TRANSMIT    = 0x95,
    ITEM_RECEIVE     = 0x96,
    ITEM_AUTO_DEPLOY = 0x97,

    ITEM_ACTIVE_LAST,


    // -------------------------------------------------------------------------
    // Logistics
    // -------------------------------------------------------------------------

    ITEM_LOGISTICS_FIRST = 0xF0,
    ITEM_WORKER          = 0xF0,
    ITEM_BULLET          = 0xF1,
    ITEM_LOGISTICS_LAST,


    ITEM_MAX = ITEM_LOGISTICS_LAST,
};

static_assert(sizeof(enum item) == 1);


// -----------------------------------------------------------------------------
// Categories
// -----------------------------------------------------------------------------

enum
{
    ITEMS_NATURAL_LEN   = ITEM_SYNTH_FIRST    - ITEM_NATURAL_FIRST,
    ITEMS_SYNTH_LEN     = ITEM_ELEM_LAST      - ITEM_SYNTH_FIRST,
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

inline size_t item_str(enum item item, char *dst, size_t len)
{
    const struct im_config *config = im_config(item);
    size_t len = legion_min(len-1, config->str_len);
    memcpy(dst, im_config(item)->str, len);
    dst[len] = 0;
    return len;
}

inline const char *item_str_c(enum item item)
{
    return im_config(item)->str;
}
