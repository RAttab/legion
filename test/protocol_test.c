/* state_test.c
   Rémi Attab (remi.attab@gmail.com), 07 Nov 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/mod.h"
#include "vm/atoms.h"
#include "game/log.h"
#include "game/gen.h"
#include "game/save.h"
#include "game/tape.h"
#include "game/chunk.h"
#include "game/world.h"
#include "game/protocol.h"
#include "game/sys.h"
#include "items/io.h"
#include "items/config.h"
#include "utils/vec.h"
#include "utils/hset.h"

#include <unistd.h>


void vec64_dbg(const struct vec64 *vec, const char *name)
{
    char buffer[1024] = {0};

    char *it = buffer;
    const char *end = it + sizeof(buffer);
    it += snprintf(it, end - it, "vec(%s, %u): { ", name, vec->len);

    for (size_t i = 0; i < vec->len; ++i)
        it += snprintf(it, end - it, "%lx ", vec->vals[i]);

    it += snprintf(it, end - it, "}\n");

    fprintf(stderr, "%s", buffer);
}

void check(void)
{
    enum { attempts = 5, steps = 100 };

    sys_populate_tests();
    const user_t user = user_admin;
    struct world *world = world_new(0);
    world_populate(world);
    struct coord home = world_home(world, user);

    world_step(world); // to create all the objects.

    struct save *save = save_mem_new();
    assert(save);

    struct state *state = state_alloc();
    assert(state);

    {
        struct symbol name = make_symbol("boot");
        struct mods *mods = world_mods(world);
        const struct mod *mod = mods_latest(mods, mods_find(mods, &name));
        assert(mod);

        word arg = mod->id;
        struct chunk *chunk = world_chunk(world, home);
        bool ok = chunk_io(chunk, IO_MOD, 0, make_id(ITEM_BRAIN, 1), &arg, 1);
        assert(ok);
    }

    struct ack *ack = ack_new();
    for (size_t attempt = 0; attempt < attempts; ++attempt) {
        world_log_push(world, user, make_coord(1, 1), make_id(1, 1), IO_PING, IOE_INVALID_STATE);
        world_log_push(world, user, make_coord(2, 2), make_id(2, 2), IO_NAME, IOE_MISSING_ARG);

        save_mem_reset(save);
        state_save(save, &(struct state_ctx) {
                    .world = world,
                    .access = user_to_uset(user),
                    .user = user,
                    .speed = speed_fast,
                    .chunk = home,
                    .ack = ack,
                });

        save_mem_reset(save);
        assert(state_load(state, save, ack));

        assert(state->seed == world_seed(world));
        assert(state->time == world_time(world));
        assert(coord_eq(state->home, home));

        {
            struct vec64 *wd_atoms = atoms_list(world_atoms(world));
            struct vec64 *st_atoms = atoms_list(state->atoms);
            assert(vec64_eq(wd_atoms, st_atoms));
            vec64_free(wd_atoms);
            vec64_free(st_atoms);
        }

        struct mods_list *wd_mods = mods_list(world_mods(world), user_to_uset(user));
        struct mods_list *st_mods = state->mods;
        assert(wd_mods->len == st_mods->len);
        for (size_t i = 0; i < wd_mods->len; ++i) {
            assert(wd_mods->items[i].maj == st_mods->items[i].maj);
            assert(wd_mods->items[i].ver == st_mods->items[i].ver);
            assert(symbol_eq(&wd_mods->items[i].str, &st_mods->items[i].str));
        }
        free(wd_mods);

        {
            struct vec64 *chunks = world_chunk_list(world);
            assert(vec64_eq(chunks, state->chunks));
            vec64_free(chunks);
        }

        for (const struct htable_bucket *it = htable_next(&state->lanes, NULL);
             it; it = htable_next(&state->lanes, it))
        {
            const struct hset *wd_lanes = world_lanes_list(world, coord_from_u64(it->key));
            const struct hset *st_lanes = (void *) it->value;
            assert(hset_eq(wd_lanes, st_lanes));
        }

        for (const struct htable_bucket *it = htable_next(&state->names, NULL);
             it; it = htable_next(&state->names, it))
        {
            word name = world_star_name(world, coord_from_u64(it->key));
            assert(name && name == (word) it->value);
        }

        {
            const struct tech *tech = world_tech(world, user);
            assert(tape_set_eq(&tech->known, &state->tech.known));
            assert(tape_set_eq(&tech->learned, &state->tech.learned));
            assert(htable_eq(&tech->research, &state->tech.research));
        }

        const struct logi *wd_log = log_next(world_log(world, user), NULL);
        const struct logi *st_log = log_next(state->log, NULL);
        while (wd_log) {
            assert(coord_eq(wd_log->star, st_log->star));
            assert(wd_log->time == st_log->time);
            assert(wd_log->id == st_log->id);
            assert(wd_log->key == st_log->key);
            assert(wd_log->value == st_log->value);

            wd_log = log_next(world_log(world, user), wd_log);
            st_log = log_next(state->log, st_log);
        }
        assert(wd_log == st_log);

        {
            struct chunk *wd_chunk = world_chunk(world, home);
            struct chunk *st_chunk = state->chunk.chunk;

            assert(coord_eq(state->chunk.coord, home));
            assert(coord_eq(chunk_star(st_chunk)->coord, home));

            struct vec64 *wd_items = chunk_list(wd_chunk);
            struct vec64 *st_items = chunk_list(st_chunk);
            assert(vec64_eq(wd_items, st_items));
            vec64_free(wd_items);
            vec64_free(st_items);
        }

        for (size_t step = 0; step < steps; ++step) world_step(world);
    }

    ack_free(ack);
    state_free(state);
    save_mem_free(save);
    world_free(world);
}


int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    check();

    return 0;
}
