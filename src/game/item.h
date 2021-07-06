/* item.h
   Rémi Attab (remi.attab@gmail.com), 29 Nov 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"


// -----------------------------------------------------------------------------
// item
// -----------------------------------------------------------------------------

legion_packed enum item
{
    ITEM_NIL = 0x00,

    // Natural
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

    // Synth
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

    // ... moi j'connai mon alphabet ...

    // Passives
    ITEM_PASSIVE_FIRST = 0x20,
    ITEM_FRAME         = 0x21,
    ITEM_GEAR          = 0x22,
    ITEM_FUEL          = 0x23,
    ITEM_BONDING       = 0x24,
    ITEM_CIRCUIT       = 0x26,
    ITEM_NEURAL        = 0x27,
    ITEM_SERVO         = 0x40,
    ITEM_THRUSTER      = 0x41,
    ITEM_PROPULSION    = 0x42,
    ITEM_PLATE         = 0x43,
    ITEM_SHIELDING     = 0x44,
    ITEM_HULL_I        = 0x4A,
    ITEM_CORE          = 0x50,
    ITEM_MATRIX        = 0x51,
    ITEM_DATABANK      = 0x52,
    ITEM_PASSIVE_LAST,

    // Logistics
    ITEM_LOGISTIC_FIRST = 0xA0,
    ITEM_WORKER         = 0xA0,
    ITEM_SHUTTLE_S      = 0xA1,
    ITEM_SHUTTLE_M      = 0xA2,
    ITEM_SHUTTLE_F      = 0xA3,
    ITEM_LOGISTIC_LAST,

    // Actives
    ITEM_ACTIVE_FIRST  = 0xB0,
    ITEM_DEPLOY        = 0xB0,
    ITEM_EXTRACT_I     = 0xB1,
    ITEM_EXTRACT_II    = 0xB2,
    ITEM_EXTRACT_III   = 0xB3,
    ITEM_PRINTER_I     = 0xB4,
    ITEM_PRINTER_II    = 0xB5,
    ITEM_PRINTER_III   = 0xB6,
    ITEM_ASSEMBLER_I   = 0xB7,
    ITEM_ASSEMBLER_II  = 0xB8,
    ITEM_ASSEMBLER_III = 0xB9,
    ITEM_STORAGE       = 0xBA,
    ITEM_DB_I          = 0xC0,
    ITEM_DB_II         = 0xC1,
    ITEM_DB_III        = 0xC2,
    ITEM_BRAIN_I       = 0xC3,
    ITEM_BRAIN_II      = 0xC4,
    ITEM_BRAIN_III     = 0xC5,
    ITEM_LEGION_I      = 0xC6,
    ITEM_LEGION_II     = 0xC7,
    ITEM_LEGION_III    = 0xC8,
    ITEM_ACTIVE_LAST,

    ITEM_MAX = ITEM_ACTIVE_LAST,
};

static_assert(sizeof(enum item) == 1);

enum items_utils
{
    ITEMS_NATURAL_LEN = ITEM_SYNTH_FIRST - ITEM_NATURAL_FIRST,
    ITEMS_SYNTH_LEN = ITEM_ELEM_LAST - ITEM_SYNTH_FIRST,
    ITEMS_PASSIVE_LEN = ITEM_PASSIVE_LAST - ITEM_PASSIVE_FIRST,
    ITEMS_ACTIVE_LEN = ITEM_ACTIVE_LAST - ITEM_ACTIVE_FIRST,
    ITEMS_SPECIAL_LEN = ITEM_SPECIAL_LAST - ITEM_SPECIAL_FIRST,
};

static_assert(ITEMS_NATURAL_LEN + ITEMS_SYNTH_LEN == 26);

enum { item_str_len = 4 };
size_t item_str(enum item, size_t len, char *dst);


// -----------------------------------------------------------------------------
// id
// -----------------------------------------------------------------------------

typedef uint32_t id_t;

inline id_t make_id(enum item type, id_t id) { return type << 24 | id; }
inline enum item id_item(id_t id) { return id >> 24; }
inline uint32_t id_bot(id_t id) { return id & ((1 << 24) - 1); }

enum { id_str_len = item_str_len+1+6 };
size_t id_str(id_t id, size_t len, char *dst);
