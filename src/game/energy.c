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
    save_write(save, en, sizeof(*en));
    save_write_magic(save, save_magic_energy);
}

bool energy_load(struct energy *en, struct save *save)
{
    if (!save_read_magic(save, save_magic_energy)) return false;
    save_read(save, en, sizeof(*en));
    return save_read_magic(save, save_magic_energy);
}
