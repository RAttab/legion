/* energy.c
   Rémi Attab (remi.attab@gmail.com), 12 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/energy.h"
#include "utils/save.h"
#include "db/specs.h"


// -----------------------------------------------------------------------------
// energy
// -----------------------------------------------------------------------------

void energy_save(const struct energy *en, struct save *save)
{
    save_write_magic(save, save_magic_energy);
    save_write(save, en, sizeof(*en));
    save_write_magic(save, save_magic_energy);
}

bool energy_load(struct energy *en, struct save *save)
{
    if (!save_read_magic(save, save_magic_energy)) return false;
    save_read(save, en, sizeof(*en));
    return save_read_magic(save, save_magic_energy);
}


im_energy energy_battery_cap(const struct energy *en)
{
    return en->battery * im_battery_storage_cap;
}


im_energy energy_solar_output(im_energy star, size_t solar)
{
    return (star * solar) / im_solar_energy_div;
}

im_energy energy_prod_solar(
        const struct energy *en, const struct star *star)
{
    return energy_solar_output(star->energy, en->solar);
}


im_energy energy_kwheel_output(uint8_t elem_k, size_t kwheel)
{
    return (elem_k * kwheel) / im_kwheel_energy_div;
}

im_energy energy_prod_kwheel(
        const struct energy *en, const struct star *star)
{
    const uint16_t elem_k = star_scan(star, item_elem_k);
    return energy_kwheel_output(elem_k, en->kwheel);
}

bool energy_consume(struct energy *en, im_energy value)
{
    en->need += value;

    if (en->consumed + value > en->produced) return false;
    en->consumed += value;

    assert(en->consumed <= en->produced);
    return true;
}

void energy_produce_burner(struct energy *en, im_energy produced)
{
    en->produced += produced;
    en->item.burner += produced;
}

void energy_step_begin(struct energy *en, const struct star *star)
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
im_energy energy_step_fusion(
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

void energy_step_end(struct energy *en)
{
    im_energy excess = en->produced - en->consumed;
    en->item.battery.stored = legion_min(excess, energy_battery_cap(en));
}
