/* save_test.c
   Rémi Attab (remi.attab@gmail.com), 26 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/mod.h"
#include "vm/atoms.h"
#include "utils/save.h"
#include "game/game.h"
#include "db/io.h"
#include "items/config.h"
#include "utils/vec.h"

#include <unistd.h>


void check_file(const char *path)
{
    enum { attempts = 5, steps = 100 };

    sys_populate_tests();
    struct world *old = world_new(0);
    world_populate(old);
    struct coord coord = world_home(old, user_admin);

    world_step(old); // to create all the objects.

    {
        struct symbol name = make_symbol("boot");
        struct mods *mods = world_mods(old);
        const struct mod *mod = mods_latest(mods, mods_find(mods, &name));
        assert(mod);

        vm_word arg = mod->id;
        struct chunk *chunk = world_chunk(old, coord);
        bool ok = chunk_io(chunk, io_mod, 0, make_im_id(item_brain, 1), &arg, 1);
        assert(ok);
    }

    for (size_t attempt = 0; attempt < attempts; ++attempt) {
        {
            struct save *save = save_file_create(path, 1);
            assert(save);
            world_save(old, save);
            save_file_close(save);
        }

        struct world *new = NULL;
        {
            struct save *save = save_file_load(path);
            assert(save);
            new = world_load(save);
            save_file_close(save);
        }

        assert(new);

        for (enum item item = 0; item < items_max; ++item)
            assert(world_scan(old, coord, item) <= world_scan(new, coord, item));

        struct vec64 *old_atoms = atoms_list(world_atoms(old));
        struct vec64 *new_atoms = atoms_list(world_atoms(new));
        assert(vec64_eq(old_atoms, new_atoms));
        vec64_free(old_atoms);
        vec64_free(new_atoms);

        struct mods_list *old_mods = mods_list(world_mods(old), user_set_all());
        struct mods_list *new_mods = mods_list(world_mods(new), user_set_all());
        assert(old_mods->len == new_mods->len);
        for (size_t i = 0; i < old_mods->len; ++i) {
            assert(old_mods->items[i].maj == new_mods->items[i].maj);
            assert(symbol_eq(&old_mods->items[i].str, &new_mods->items[i].str));
        }
        free(old_mods);
        free(new_mods);

        world_free(old);
        old = new;
        for (size_t step = 0; step < steps; ++step) world_step(old);
    }

    world_free(old);
}

void check_ring(void)
{
    enum {
        ring_cap = s_page_len,
        partial_len = ring_cap / 2 + 107,
    };

    struct save_ring *ring = save_ring_new(ring_cap);
    assert(ring);

    for (uint8_t attempt = 0; attempt < 255; ++attempt) {
        {
            assert(save_cap(save_ring_read(ring)) == 0);

            struct save *save = save_ring_write(ring);
            assert(save_cap(save) == ring_cap);

            uint8_t data[ring_cap] = {0};
            memset(data, attempt, sizeof(data));

            size_t ret = save_write(save, data, sizeof(data));
            assert(ret == sizeof(data));

            save_ring_commit(ring, save);
        }

        {
            assert(save_cap(save_ring_write(ring)) == 0);

            struct save *save = save_ring_read(ring);
            assert(save_cap(save) == ring_cap);

            uint8_t data[ring_cap] = {0};

            size_t ret = save_read(save, data, sizeof(data));
            assert(ret == sizeof(data));

            for (size_t i = 0; i < sizeof(data); ++i)
                assert(data[i] == attempt);

            save_ring_commit(ring, save);
        }

        const uint8_t value = attempt + 128;

        {
            assert(save_cap(save_ring_read(ring)) == 0);

            struct save *save = save_ring_write(ring);
            assert(save_cap(save) == ring_cap);

            uint8_t data[partial_len] = {0};
            memset(data, value, sizeof(data));

            size_t ret = save_write(save, data, sizeof(data));
            assert(ret == sizeof(data));

            save_ring_commit(ring, save);
        }

        {
            assert(save_cap(save_ring_write(ring)) == ring_cap - partial_len);

            struct save *save = save_ring_read(ring);
            assert(save_cap(save) == partial_len);

            uint8_t data[partial_len] = {0};

            size_t ret = save_read(save, data, sizeof(data));
            assert(ret == sizeof(data));

            for (size_t i = 0; i < sizeof(data); ++i)
                assert(data[i] == value);

            save_ring_commit(ring, save);
        }
    }

    assert(save_cap(save_ring_read(ring)) == 0);
    assert(save_cap(save_ring_write(ring)) == ring_cap);

    save_ring_free(ring);
}

int main(int argc, char **argv)
{
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/test/test_save.legion", argc > 1 ? argv[1] : ".");
    (void) unlink(path);

    check_file(path);
    check_ring();

    return 0;
}
