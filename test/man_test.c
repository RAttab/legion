/* man_test.c
   Rémi Attab (remi.attab@gmail.com), 11 May 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm/vm.h"
#include "game/game.h"
#include "items/config.h"
#include "engine/engine.h"


void check_toc(const struct toc *toc, struct lisp *lisp)
{
    struct link link = toc->link;
    if (link.page && !link.section)
        man_free(man_open(link.page, 90, lisp));

    for (size_t i = 0; i < toc->len; ++i)
        check_toc(toc->nodes + i, lisp);
}

int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    engine_populate_tests();
    struct atoms *atoms = atoms_new();
    im_populate_atoms(atoms);
    struct mods_list *mods = calloc(1, sizeof(*mods));
    struct lisp *lisp = lisp_new(mods, atoms);

    check_toc(man_toc(), lisp);

    lisp_free(lisp);
    atoms_free(atoms);
    free(mods);

    return 0;
}
