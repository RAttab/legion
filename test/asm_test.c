/* asm_test.c
   Rémi Attab (remi.attab@gmail.com), 03 Sep 2023
   FreeBSD-style copyright and disclaimer apply

   This test is meant to be run manually as it requires eye-balling the
   output. I don't think it's worth automating at the moment as it would be a
   bigger hassle then anything else.
*/

#include "vm/asm.h"
#include "vm/mod.h"
#include "vm/atoms.h"
#include "utils/fs.h"

int main(int argc, const char *argv[])
{
    (void) argc, (void) argv;

    struct mfile file = mfile_open("./res/mods/ast.lisp");

    mod_compiler_init();

    struct mods *mods = mods_new();
    struct atoms *atoms = atoms_new();
    struct mod *mod = mod_compile(1, file.ptr, file.len, mods, atoms);

    struct assembly *as = asm_alloc();
    asm_parse(as, mod);
    asm_dump(as);
    asm_free(as);

    mod_free(mod);
    atoms_free(atoms);
    mods_free(mods);

    mfile_close(&file);
}
