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

typedef uint32_t im_energy;

struct energy
{
    uint8_t solar, kwheel, battery;
    im_energy produced, consumed, need;

    struct {
        im_energy burner;
        struct { im_energy produced, stored; } battery;
        struct { im_energy produced, saved, next; } fusion;
    } item;
};


inline im_energy energy_battery_cap(const struct energy *en)
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

inline bool energy_consume(struct energy *en, im_energy value)
{
    en->need += value;

    if (en->consumed + value > en->produced) return false;
    en->consumed += value;

    assert(en->consumed <= en->produced);
    return true;
}

inline void energy_produce_burner(struct energy *en, im_energy produced)
{
    en->produced += produced;
    en->item.burner += produced;
}

inline void energy_step_begin(struct energy *en, const struct star *star)
{
    en->need = 0;
    en->produced = 0;
    en->consumed = 0;
    en->item.burner = 0;
    en->item.fusion.saved = 0;
    en->item.fusion.produced = legion_xchg(&en->item.fusion.next, 0);
    en->item.battery.produced = legion_xchg(&en->item.battery.stored, 0);

    en->produced =
        en->item.fusion.produced +
        en->item.battery.produced +
        energy_prod_solar(en, star) +
        energy_prod_kwheel(en, star);
}

// runs right before energy_step_end.
inline im_energy energy_step_fusion(
        struct energy *en, im_energy produced, im_energy cap)
{
    if (!produced) return 0;
    en->item.fusion.next += produced;

    im_energy save = en->produced - en->consumed;
    save -= legion_min(save, energy_battery_cap(en));
    save -= legion_min(save, en->item.fusion.saved);
    save = legion_min(save, cap);

    en->item.fusion.saved += save;
    return save;
}

inline void energy_step_end(struct energy *en)
{
    im_energy excess = en->produced - en->consumed;
    en->item.battery.stored = legion_min(excess, energy_battery_cap(en));
}


void energy_save(const struct energy *, struct save *);
bool energy_load(struct energy *, struct save *);
