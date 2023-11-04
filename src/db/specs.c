/* specs.c
   Rémi Attab (remi.attab@gmail.com), 19 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/


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
    struct spec_data map[spec_max];
    struct symbol atoms[spec_max];
} specs = {0};


struct specs_ret specs_var(enum spec spec)
{
    if (unlikely(spec >= spec_max))
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

    if (unlikely(spec >= spec_max))
        return (struct specs_ret) { .ok = false };

    if (unlikely(specs.map[spec].type != spec_type_fn))
        return (struct specs_ret) { .ok = false };

    return specs.map[spec].data.fn(args, len);
}

// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

#include "specs_fn.c"

void specs_populate(void)
{

#define spec_register_var(_spec, _atom, _value)                 \
    do {                                                        \
        specs.map[_spec] = (struct spec_data) {                 \
            .type = spec_type_var, .data = { .var = _value } }; \
        specs.atoms[_spec] = make_symbol(_atom);                \
    } while(false)

#define spec_register_fn(_spec, _atom, _value)                  \
    do {                                                        \
        specs.map[_spec] = (struct spec_data) {                 \
            .type = spec_type_fn, .data = { .fn = _value } };   \
        specs.atoms[_spec] = make_symbol(_atom);                \
    } while(false)

    spec_register_var(spec_star_item_cap, "spec-star-item-cap", chunk_item_cap);
    spec_register_fn(spec_stars_travel_time, "spec-stars-travel-time", spec_stars_travel_time_fn);

    spec_register_var(spec_test_var, "spec-test-var", 0x123);
    spec_register_fn(spec_test_fn, "spec-test-fn", spec_test_fn_fn);

    #include "gen/specs_register.h"

#undef spec_register_var
#undef spec_register_fn
}

void specs_populate_atoms(struct atoms *atoms)
{
    for (enum spec spec = spec_nil; spec < spec_max; ++spec) {
        if (!specs.map[spec].type) continue;
        atoms_set(atoms, specs.atoms + spec, specs_atom_base + spec);
    }
}
