/* mod_test.c
   Rémi Attab (remi.attab@gmail.com), 27 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "vm.h"
#include "engine.h"

struct mod *compile(const char *code)
{
    struct text text = {0};
    text_from_str(&text, code, strlen(code));

    struct mod *mod =  mod_compile(&text, NULL);
    for (size_t i = 0; i < mod->errs_len; ++i)
        dbg("%zu: %s", mod->errs[i].line, mod->errs[i].str);

    return mod;
}

const char *fib =
    "push 1\n"
    "push 1\n"
    "popr $1\n"
    "\n"
    "loop:\n"
    "pushr $1\n"
    "swap\n"
    "dupe\n"
    "popr $1\n"
    "add\n"
    "yield\n"
    "jmp @loop\n";

void test_compiler(void)
{
    {
        struct mod *mod = compile("asd");
        assert(mod);
        assert(mod->errs_len == 1);
        mod_free(mod);
    }

    {
        struct mod *mod = compile(fib);
        assert(mod);
        assert(mod->errs_len == 0);
        mod_free(mod);
    }
}


int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    engine_populate_tests();

    test_compiler();

    return 0;
}
