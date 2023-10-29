/* specs_fn.c
   Rémi Attab (remi.attab@gmail.com), 20 Oct 2022
   FreeBSD-style copyright and disclaimer apply

   included from db/specs.c
*/

#include "game/chunk.h"
#include "game/lanes.h"
#include "game/energy.h"

// -----------------------------------------------------------------------------
// stars
// -----------------------------------------------------------------------------

static struct specs_ret spec_stars_travel_time_fn(const vm_word *args, size_t len)
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


// -----------------------------------------------------------------------------
// solar
// -----------------------------------------------------------------------------

static struct specs_ret spec_solar_energy_fn(const vm_word *args, size_t len)
{
    if (len < 1)
        return (struct specs_ret) { .ok = false };

    vm_word star = args[0];
    if (star < 0 || (size_t) star > star_elem_cap)
        return (struct specs_ret) { .ok = false };

    vm_word solar = len >= 2 ? args[1] : 1;
    if (solar < 0 || (size_t) solar > chunk_item_cap)
        return (struct specs_ret) { .ok = false };

    return (struct specs_ret) {
        .ok = true,
        .word = energy_solar_output(star, solar),
    };
}


// -----------------------------------------------------------------------------
// burner
// -----------------------------------------------------------------------------

static struct specs_ret spec_burner_energy_fn(const vm_word *args, size_t len)
{
    if (len < 1)
        return (struct specs_ret) { .ok = false };

    if (!item_validate(args[0]))
        return (struct specs_ret) { .ok = false };
    enum item item = args[0];

    return (struct specs_ret) {
        .ok = true,
        .word = im_burner_energy(item),
    };
}

static struct specs_ret spec_burner_work_cap_fn(const vm_word *args, size_t len)
{
    if (len < 1)
        return (struct specs_ret) { .ok = false };

    if (!item_validate(args[0]))
        return (struct specs_ret) { .ok = false };
    enum item item = args[0];

    return (struct specs_ret) {
        .ok = true,
        .word = im_burner_work_cap(item),
    };
}


// -----------------------------------------------------------------------------
// collider
// -----------------------------------------------------------------------------

static struct specs_ret spec_collider_output_rate_fn(const vm_word *args, size_t len)
{
    if (len < 1)
        return (struct specs_ret) { .ok = false };

    vm_word size = legion_min(args[0], im_collider_grow_max);
    if (size <= 0) size = 1;

    return (struct specs_ret) {
        .ok = true,
        .word = im_collider_rate(size),
    };
}


// -----------------------------------------------------------------------------
// prober
// -----------------------------------------------------------------------------

static struct specs_ret spec_prober_work_cap_fn(const vm_word *args, size_t len)
{
    if (len < 2)
        return (struct specs_ret) { .ok = false };

    if (!coord_validate(args[0]))
        return (struct specs_ret) { .ok = false };
    struct coord origin = coord_from_u64(args[0]);

    if (!coord_validate(args[1]))
        return (struct specs_ret) { .ok = false };
    struct coord target = coord_from_u64(args[1]);

    return (struct specs_ret) {
        .ok = true,
        .word = im_prober_work_cap(origin, target),
    };
}


// -----------------------------------------------------------------------------
// scanner
// -----------------------------------------------------------------------------

static struct specs_ret spec_scanner_work_cap_fn(const vm_word *args, size_t len)
{
    if (len < 2)
        return (struct specs_ret) { .ok = false };

    if (!coord_validate(args[0]))
        return (struct specs_ret) { .ok = false };
    struct coord origin = coord_from_u64(args[0]);

    if (!coord_validate(args[1]))
        return (struct specs_ret) { .ok = false };
    struct coord target = coord_from_u64(args[1]);

    return (struct specs_ret) {
        .ok = true,
        .word = im_scanner_work_cap(origin, target),
    };
}


// -----------------------------------------------------------------------------
// misc
// -----------------------------------------------------------------------------

static struct specs_ret spec_test_fn_fn(const vm_word *args, size_t len)
{
    if (len != 2)
        return (struct specs_ret) { .ok = false };

    return (struct specs_ret) {
        .ok = true,
        .word = args[0] - args[1],
    };
}
