/* specs.c
   Rémi Attab (remi.attab@gmail.com), 19 Sep 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/specs.h"
#include "game/energy.h"
#include "vm/symbol.h"

#include "items/storage/storage.h"
#include "items/legion/legion.h"
#include "items/port/port.h"

// -----------------------------------------------------------------------------
// specs
// -----------------------------------------------------------------------------

enum spec_type { spec_type_nil = 0, spec_type_var, spec_type_fn };
typedef struct specs_ret (*spec_fn) (const vm_word *args, size_t len);

struct spec_data
{
    enum spec_type type;
    union { vm_word var; spec_fn fn; } data;
};

static struct
{
    struct spec_data map[SPEC_MAX];
    struct symbol atoms[SPEC_MAX];
} specs = {0};


struct specs_ret specs_var(enum spec spec)
{
    if (unlikely(spec >= SPEC_MAX))
        return (struct specs_ret) { .ok = false };

    if (unlikely(specs.map[spec].type != spec_type_var))
        return (struct specs_ret) { .ok = false };

    return (struct specs_ret) {
        .ok = true,
        .word = specs.map[spec].data.var,
    };
}

struct specs_ret specs_args(enum spec spec, const vm_word *args, size_t len)
{
    if (likely(!len)) return specs_var(spec);

    if (unlikely(spec >= SPEC_MAX))
        return (struct specs_ret) { .ok = false };

    if (unlikely(specs.map[spec].type != spec_type_fn))
        return (struct specs_ret) { .ok = false };

    return specs.map[spec].data.fn(args, len);
}


// -----------------------------------------------------------------------------
// spec fn
// -----------------------------------------------------------------------------

static struct specs_ret spec_stars_travel_time(const vm_word *args, size_t len)
{
    if (len < 3)
        return (struct specs_ret) { .ok = false };

    if (args[0] <= 0)
        return (struct specs_ret) { .ok = false };
    size_t speed = legion_min(args[0], 1);

    if (!coord_validate(args[1]))
        return (struct specs_ret) { .ok = false };
    struct coord src = coord_from_u64(args[1]);

    if (!coord_validate(args[2]))
        return (struct specs_ret) { .ok = false };
    struct coord dst = coord_from_u64(args[2]);

    return (struct specs_ret) {
        .ok = true,
        .word = lanes_travel(speed, src, dst),
    };
}

static struct specs_ret spec_solar_energy(const vm_word *args, size_t len)
{
    if (len < 1)
        return (struct specs_ret) { .ok = false };

    vm_word star = args[0];
    if (star < 0 || star > star_elem_cap)
        return (struct specs_ret) { .ok = false };

    vm_word solar = len >= 2 ? args[1] : 1;
    if (solar < 0 || solar > chunk_item_cap)
        return (struct specs_ret) { .ok = false };

    return (struct specs_ret) {
        .ok = true,
        .word = energy_solar_output(star, solar),
    };
}

static struct specs_ret spec_kwheel_energy(const vm_word *args, size_t len)
{
    if (len < 1)
        return (struct specs_ret) { .ok = false };

    vm_word elem_k = args[0];
    if (elem_k < 0 || elem_k > star_elem_cap)
        return (struct specs_ret) { .ok = false };

    vm_word kwheel = len >= 2 ? args[1] : 1;
    if (kwheel < 0 || kwheel > chunk_item_cap)
        return (struct specs_ret) { .ok = false };

    return (struct specs_ret) {
        .ok = true,
        .word = energy_kwheel_output(elem_k, kwheel),
    };
}

static struct specs_ret spec_test_fn(const vm_word *args, size_t len)
{
    if (len != 2)
        return (struct specs_ret) { .ok = false };

    return (struct specs_ret) {
        .ok = true,
        .word = args[0] - args[1],
    };
}


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

static void specs_register_atom(enum spec spec, const char *str)
{
    struct symbol *atom = specs.atoms + spec;
    *atom = make_symbol(str);

    for (size_t i = 0; i < atom->len; ++i) {
        if (atom->c[i] == '_') atom->c[i] = '-';
        else atom->c[i] = tolower(atom->c[i]);
    }
}

static void specs_register_var(enum spec spec, const char *str, vm_word value)
{
    assert(spec < SPEC_MAX);
    specs_register_atom(spec, str);
    specs.map[spec] = (struct spec_data) {
        .type = spec_type_var,
        .data = { .var = value },
    };
}

static void specs_register_fn(enum spec spec, const char *str, spec_fn value)
{
    assert(spec < SPEC_MAX);
    specs_register_atom(spec, str);
    specs.map[spec] = (struct spec_data) {
        .type = spec_type_fn,
        .data = { .fn = value },
    };
}


void specs_populate(void)
{
#define register_var(spec, value) specs_register_var(spec, #spec, value)
#define register_fn(spec, value) specs_register_fn(spec, #spec, value)

    register_var(SPEC_STAR_ITEM_CAP, chunk_item_cap);
    register_fn(SPEC_STARS_TRAVEL_TIME, spec_stars_travel_time);

    register_var(SPEC_LEGION_TRAVEL_SPEED, im_legion_speed);
    register_var(SPEC_STORAGE_CAP, im_storage_max);
    register_var(SPEC_PORT_LAUNCH_SPEED, im_port_speed);

    register_fn(SPEC_SOLAR_ENERGY, spec_solar_energy),
    register_fn(SPEC_KWHEEL_ENERGY, spec_kwheel_energy),

    register_var(SPEC_TEST_VAR, 0x123);
    register_fn(SPEC_TEST_FN, spec_test_fn);

#undef register_var
#undef register_fn
}

void specs_populate_atoms(struct atoms *atoms)
{
    for (enum spec spec = SPEC_NIL; spec < SPEC_MAX; ++spec) {
        if (!specs.map[spec].type) continue;
        atoms_set(atoms, specs.atoms + spec, specs_atom_base + spec);
    }
}
