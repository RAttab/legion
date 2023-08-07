/* ast_test.c
   Rémi Attab (remi.attab@gmail.com), 28 Aug 2023
   FreeBSD-style copyright and disclaimer apply

   This test is meant to be run manually as it requires eye-balling the
   output. I don't think it's worth automating at the moment as it would be a
   bigger hassle then anything else.
*/

#include "vm/ast.h"
#include "utils/fs.h"

int main(int argc, const char *argv[])
{
    (void) argc, (void) argv;

    ast_populate();
    struct mfile file = mfile_open("./res/mods/ast.lisp");

    struct ast *ast = ast_alloc();
    ast_parse(ast, file.ptr, file.len);
    ast_dump(ast);

    ast_free(ast);
    mfile_close(&file);
}
