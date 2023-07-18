/* specs.h
   Rémi Attab (remi.attab@gmail.com), 20 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/vm.h"
#include "db/items.h"
#include "game/tape.h"
#include "game/energy.h"

#include "gen/specs_value.h"

struct atoms;

// -----------------------------------------------------------------------------
// enum spec
// -----------------------------------------------------------------------------

enum
{
    specs_atom_base = 1 << 12,
    specs_max_args = 3,

    spec_lab_bits = 0xD,
    spec_lab_work = 0xE,
    spec_lab_energy = 0xF,
};

#define make_spec(item, type) ((item << 4) | type)

enum spec : uint16_t
{
    spec_nil = 0,

    spec_star_item_cap     = make_spec(item_nil, 0x0),
    spec_stars_travel_time = make_spec(item_nil, 0x1),

    spec_test_var          = make_spec(item_nil, 0xE),
    spec_test_fn           = make_spec(item_nil, 0xF),

    #include "gen/specs_enum.h"

    spec_max = make_spec(items_max, 0x00),
};

inline enum item spec_item(enum spec spec) { return spec >> 4; }
inline uint8_t spec_type(enum spec spec) { return spec & 0xF; }

inline bool spec_validate(vm_word word)
{
    return word > 0 && (word - specs_atom_base) < spec_max;
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

inline vm_word specs_var_assert(enum spec spec)
{
    struct specs_ret ret = specs_var(spec);
    assert(ret.ok);
    return ret.word;
}


void specs_populate(void);
void specs_populate_atoms(struct atoms *);
