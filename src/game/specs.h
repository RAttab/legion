/* specs.h
   Rémi Attab (remi.attab@gmail.com), 19 Sep 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "items/item.h"
#include "vm/vm.h"

struct atoms;


// -----------------------------------------------------------------------------
// enum spec
// -----------------------------------------------------------------------------

enum
{
    specs_atom_base = 1 << 12,
    specs_max_args = 3,

    spec_lab_bits = 0xF,
};

#define make_spec(item, type) ((item << 4) | type)

enum spec
{
    SPEC_NIL = 0,

    SPEC_STAR_ITEM_CAP     = make_spec(ITEM_NIL, 0x0),
    SPEC_STARS_TRAVEL_TIME = make_spec(ITEM_NIL, 0x1),

    SPEC_LEGION_TRAVEL_SPEED = make_spec(ITEM_LEGION, 0x0),
    SPEC_STORAGE_CAP         = make_spec(ITEM_STORAGE, 0x0),
    SPEC_PORT_LAUNCH_SPEED   = make_spec(ITEM_PORT, 0x0),

    SPEC_SOLAR_ENERGY  = make_spec(ITEM_SOLAR, 0x0),
    SPEC_KWHEEL_ENERGY = make_spec(ITEM_KWHEEL, 0x0),

    SPEC_TEST_VAR          = make_spec(0xFF, 0x0),
    SPEC_TEST_FN           = make_spec(0xFF, 0x1),

    SPEC_MAX,
};

inline enum item spec_item(enum spec spec) { return spec >> 4; }
inline uint8_t spec_type(enum spec spec) { return spec & 0xF; }

inline bool spec_validate(vm_word word)
{
    return word > 0 && (word - specs_atom_base) < SPEC_MAX;
}

inline enum spec spec_from_word(vm_word word)
{
    return word - specs_atom_base;
}


// -----------------------------------------------------------------------------
// specs
// -----------------------------------------------------------------------------

struct specs_ret { bool ok; vm_word word; };

struct specs_ret specs_var(enum spec);
struct specs_ret specs_args(enum spec, const vm_word *args, size_t len);

void specs_populate(void);
void specs_populate_atoms(struct atoms *);
