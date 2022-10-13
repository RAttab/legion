/* energy.h
   Rémi Attab (remi.attab@gmail.com), 23 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/sector.h"

struct save;


// -----------------------------------------------------------------------------
// specs
// -----------------------------------------------------------------------------

enum
{
    energy_battery_mul = 1000,
    energy_solar_div = 1000,
    energy_kwheel_div = 10,
};


// -----------------------------------------------------------------------------
// energy
// -----------------------------------------------------------------------------

typedef uint64_t im_energy;

struct energy
{
    uint8_t solar, kwheel, battery;

    im_energy current;
    im_energy produced, consumed, need;
    struct { im_energy burner, battery; } item;
};


inline im_energy energy_battery(const struct energy *en)
{
    return en->battery * energy_battery_mul;
}


inline im_energy energy_solar_output(im_energy star, size_t solar)
{
    return (star * solar) / energy_solar_div;
}

inline im_energy energy_prod_solar(
        const struct energy *en, const struct star *star)
{
    return energy_solar_output(star->energy, en->solar);
}


inline im_energy energy_kwheel_output(uint8_t elem_k, size_t kwheel)
{
    return (elem_k * kwheel) / energy_kwheel_div;
}

inline im_energy energy_prod_kwheel(
        const struct energy *en, const struct star *star)
{
    const uint16_t elem_k = star_scan(star, ITEM_ELEM_K);
    return energy_kwheel_output(elem_k, en->kwheel);
}


inline im_energy energy_production(
        const struct energy *en, const struct star *star)
{
    return
        energy_prod_solar(en, star) +
        energy_prod_kwheel(en, star);
}


inline bool energy_consume(struct energy *en, im_energy value)
{
    en->need += value;
    if (value > en->current) return false;

    en->current -= value;
    en->consumed += value;
    return true;
}

inline void energy_produce(struct energy *en, im_energy value)
{
    en->current += value;
    en->produced += value;
}

inline void energy_produce_item(struct energy *en, enum item item, im_energy value)
{
    energy_produce(en, value);

    switch (item) {
    case ITEM_BURNER: { en->item.burner++; break; }
    default: { assert(false); }
    }
}


inline void energy_step_begin(struct energy *en, const struct star *star)
{
    en->need = 0;
    en->produced = 0;
    en->consumed = 0;
    en->item.battery = en->current;
    energy_produce(en, energy_production(en, star));
}

inline void energy_step_end(struct energy *en)
{
    en->current = legion_min(en->current, energy_battery(en));
}


void energy_save(const struct energy *, struct save *);
bool energy_load(struct energy *, struct save *);
