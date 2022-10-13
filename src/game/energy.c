/* energy.c
   Rémi Attab (remi.attab@gmail.com), 12 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/energy.h"
#include "game/save.h"


// -----------------------------------------------------------------------------
// energy
// -----------------------------------------------------------------------------

void energy_save(const struct energy *en, struct save *save)
{
    save_write_magic(save, save_magic_energy);
    save_write_value(save, en->solar);
    save_write_value(save, en->kwheel);
    save_write_value(save, en->battery);
    save_write_value(save, en->current);
    save_write_value(save, en->produced);
    save_write_value(save, en->consumed);
    save_write_value(save, en->need);
    save_write_value(save, en->item.burner);
    save_write_value(save, en->item.battery);
    save_write_magic(save, save_magic_energy);
}

bool energy_load(struct energy *en, struct save *save)
{
    if (!save_read_magic(save, save_magic_energy)) return false;
    save_read_into(save, &en->solar);
    save_read_into(save, &en->kwheel);
    save_read_into(save, &en->battery);
    save_read_into(save, &en->current);
    save_read_into(save, &en->produced);
    save_read_into(save, &en->consumed);
    save_read_into(save, &en->need);
    save_read_into(save, &en->item.burner);
    save_read_into(save, &en->item.battery);
    return save_read_magic(save, save_magic_energy);
}
