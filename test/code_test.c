/* ast_test.c
   Rémi Attab (remi.attab@gmail.com), 28 Aug 2023
   FreeBSD-style copyright and disclaimer apply

   This test is meant to be run manually as it requires eye-balling the
   output. I don't think it's worth automating at the moment as it would be a
   bigger hassle then anything else.
*/

#include "vm/code.h"
#include "utils/fs.h"

int main(int argc, const char *argv[])
{
    (void) argc, (void) argv;

    ast_populate();
    struct mfile file = mfile_open("./res/mods/ast.lisp");

    struct code *code = code_alloc();
    code_set(code, file.ptr, file.len);
    code_dump(code);
    code_free(code);

    mfile_close(&file);
}
