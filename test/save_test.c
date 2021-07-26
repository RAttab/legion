/* save_test.c
   Rémi Attab (remi.attab@gmail.com), 26 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/io.h"
#include "game/save.h"
#include "game/prog.h"
#include "game/chunk.h"
#include "game/world.h"
#include "vm/types.h"
#include "vm/mod.h"
#include "utils/vec.h"

#include <unistd.h>


void check(const char *path)
{
    enum { attempts = 2, steps = 100 };

    prog_load();
    mod_compiler_init();
    struct world *old = world_new();
    struct coord coord = world_populate(old);
    world_step(old); // to create all the objects.

    {
        struct symbol name = make_symbol("boot");
        struct mods *mods = world_mods(old);
        const struct mod *mod = mods_latest(mods, mods_find(mods, &name));
        assert(mod);

        word_t arg = mod->id;
        struct chunk *chunk = world_chunk(old, coord);
        bool ok = chunk_io(chunk, IO_MOD, 0, make_id(ITEM_BRAIN_2, 1), 1, &arg);
        assert(ok);
    }

    for (size_t attempt = 0; attempt < attempts; ++attempt) {
        {
            struct save *save = save_new(path, 1);
            assert(save);
            world_save(old, save);
            save_close(save);
        }

        struct world *new = NULL;
        {
            struct save *save = save_load(path);
            assert(save);
            new = world_load(save);
            save_close(save);
        }

        assert(new);

        for (enum item item = 0; item < ITEM_MAX; ++item)
            assert(world_scan(old, coord, item) <= world_scan(new, coord, item));

        struct vec64 *old_atoms = atoms_list(world_atoms(old));
        struct vec64 *new_atoms = atoms_list(world_atoms(new));
        assert(vec64_eq(old_atoms, new_atoms));
        vec64_free(old_atoms);
        vec64_free(new_atoms);

        struct mods_list *old_mods = mods_list(world_mods(old));
        struct mods_list *new_mods = mods_list(world_mods(new));
        assert(old_mods->len == new_mods->len);
        for (size_t i = 0; i < old_mods->len; ++i) {
            assert(old_mods->items[i].id == new_mods->items[i].id);
            assert(symbol_eq(&old_mods->items[i].str, &new_mods->items[i].str));
        }
        free(old_mods);
        free(new_mods);

        for (size_t step = 0; step < steps; ++step) world_step(new);
        world_free(old);
        old = new;
    }

    world_free(old);
}

int main(int argc, char **argv)
{
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/test/test_save.legion", argc > 1 ? argv[1] : ".");
    (void) unlink(path);

    check(path);
    return 0;
}
