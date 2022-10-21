/* items.c
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/atoms.h"
#include "game/tape.h"
#include "db/items.h"
#include "db/tapes.h"
#include "items/config.h"
#include "render/render.h"


// -----------------------------------------------------------------------------
// items
// -----------------------------------------------------------------------------

bool stats_run(void)
{
    struct symbol sym = {0};
    struct atoms *atoms = atoms_new();
    im_populate_atoms(atoms);

    for (enum item id = 0; id < items_max; ++id) {
        const struct tape *tape = tapes_get(id);
        const struct tape_info *info = tapes_info(id);
        if (!tape || !info) continue;

        atoms_str(atoms, id, &sym);
        fprintf(stdout, "(%s\n  (rank %zu)\n  (energy %u)",
                sym.c, info->rank, tape_energy(tape));

        for (size_t i = 0; i < items_natural_len; ++i) {
            if (!info->elems[i]) continue;

            atoms_str(atoms, items_natural_first + i, &sym);
            fprintf(stdout, "\n  (%s %u)", sym.c, info->elems[i]);
        }

        for (enum item it = tape_set_next(&info->reqs, 0);
             it; it = tape_set_next(&info->reqs, it))
        {
            atoms_str(atoms, it, &sym);
            fprintf(stdout, "\n  (req %s)", sym.c);
        }

        fprintf(stdout, ")\n\n");
    }

    atoms_free(atoms);
    return true;
}
