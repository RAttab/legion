/* sys.c
   Rémi Attab (remi.attab@gmail.com), 23 Dec 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/sys.h"
#include "game/tape.h"
#include "game/gen.h"
#include "items/config.h"
#include "vm/mod.h"
#include "vm/atoms.h"


// -----------------------------------------------------------------------------
// sys
// -----------------------------------------------------------------------------

void sys_populate(void)
{
    im_populate();
    mod_compiler_init();

    {
        struct atoms *atoms = atoms_new();
        im_populate_atoms(atoms);

        gen_populate(atoms);
        tapes_populate(atoms);

        atoms_free(atoms);
    }
}

void sys_path_res(const char *name, char *dst, size_t len)
{
    snprintf(dst, len, "./res/%s", name);
}
