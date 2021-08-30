/* energy.h
   Rémi Attab (remi.attab@gmail.com), 23 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/sector.h"


// -----------------------------------------------------------------------------
// specs
// -----------------------------------------------------------------------------

enum
{
    energy_store_mul = 1000,
    energy_solar_div = 1000,
    energy_kwheel_div = 10,
};


// -----------------------------------------------------------------------------
// energy
// -----------------------------------------------------------------------------

typedef uint64_t energy_t;

struct energy
{
    uint16_t solar, kwheel, store;

    energy_t current;
    energy_t produced, consumed, need;
};


inline energy_t energy_store(const struct energy *en)
{
    return en->store * energy_store_mul;
}

inline energy_t energy_prod_solar(
        const struct energy *en, const struct star *star)
{
    return (star->energy * en->solar) / energy_solar_div;
}

inline energy_t energy_prod_kwheel(
        const struct energy *en, const struct star *star)
{
    const uint16_t elem_k = star_elem(star, ITEM_ELEM_K);
    return (elem_k * en->kwheel) / energy_kwheel_div;
}

inline energy_t energy_production(
        const struct energy *en, const struct star *star)
{
    return
        energy_prod_solar(en, star) +
        energy_prod_kwheel(en, star);
}


inline bool energy_consume(struct energy *en, energy_t value)
{
    en->need += value;
    if (value > en->current) return false;

    en->current -= value;
    en->consumed += value;
    return true;
}

inline void energy_produce(struct energy *en, energy_t value)
{
    en->current += value;
    en->produced += value;
}


inline void energy_step_begin(struct energy *en, const struct star *star)
{
    en->produced = 0;
    en->consumed = 0;
    energy_produce(en, energy_production(en, star));
}

inline void energy_step_end(struct energy *en)
{
    en->current = legion_min(en->current, energy_store(en));
}


inline void energy_save(const struct energy *en, struct save *save)
{
    save_write_magic(save, save_magic_energy);
    save_write_value(save, en->solar);
    save_write_value(save, en->kwheel);
    save_write_value(save, en->store);
    save_write_value(save, en->current);
    save_write_magic(save, save_magic_energy);
}

inline bool energy_load(struct energy *en, struct save *save)
{
    if (!save_read_magic(save, save_magic_energy)) return false;
    save_read_into(save, &en->solar);
    save_read_into(save, &en->kwheel);
    save_read_into(save, &en->store);
    save_read_into(save, &en->current);
    return save_read_magic(save, save_magic_energy);
}
