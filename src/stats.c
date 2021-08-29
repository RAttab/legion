/* stats.c
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/atoms.h"
#include "items/item.h"

#include "items/config.h"

#include "game/tape.h"

// -----------------------------------------------------------------------------
// stats
// -----------------------------------------------------------------------------

int stats_run(int argc, char **argv)
{
    (void) argc, (void) argv;

    im_populate();
    tapes_populate();

    struct symbol sym = {0};
    struct atoms *atoms = atoms_new();
    im_populate_atoms(atoms);

    for (enum item id = 0; id < ITEM_MAX; ++id) {
        const struct tape *tape = tapes_get(id);
        const struct tape_info *info = tapes_info(id);
        if (!tape || !info) continue;

        atoms_str(atoms, id, &sym);
        fprintf(stdout, "(%s\n  (rank %zu)\n  (energy %zu)",
                sym.c, info->rank, tape_energy(tape));

        for (size_t i = 0; i < ITEMS_NATURAL_LEN; ++i) {
            if (!info->elems[i]) continue;

            atoms_str(atoms, ITEM_NATURAL_FIRST + i, &sym);
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
    return 0;
}
