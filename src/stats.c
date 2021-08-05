/* stats.c
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/atoms.h"
#include "game/item.h"
#include "game/tape.h"

// -----------------------------------------------------------------------------
// stats
// -----------------------------------------------------------------------------

int stats_run(int argc, char **argv)
{
    (void) argc, (void) argv;

    tapes_populate();

    struct symbol sym = {0};
    struct atoms *atoms = atoms_new();
    atoms_register_game(atoms);

    for (enum item id = 0; id < ITEM_MAX; ++id) {
        const struct tape_stats *stats = tapes_stats(id);
        if (!stats) continue;

        atoms_str(atoms, id, &sym);
        fprintf(stdout, "(%s\n  (rank %zu)", sym.c, stats->rank);

        for (size_t i = 0; i < ITEMS_NATURAL_LEN; ++i) {
            if (!stats->elems[i]) continue;

            atoms_str(atoms, ITEM_NATURAL_FIRST + i, &sym);
            fprintf(stdout, "\n  (%s %u)", sym.c, stats->elems[i]);
        }

        for (enum item it = tape_set_next(&stats->reqs, 0);
             it; it = tape_set_next(&stats->reqs, it))
        {
            atoms_str(atoms, it, &sym);
            fprintf(stdout, "\n  (req %s)", sym.c);
        }

        fprintf(stdout, ")\n\n");
    }

    atoms_free(atoms);
    return 0;
}
