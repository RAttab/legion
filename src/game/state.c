/* state.c
   Rémi Attab (remi.attab@gmail.com), 30 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/state.h"
#include "game/log.h"
#include "game/chunk.h"
#include "game/save.h"
#include "vm/atoms.h"
#include "vm/mod.h"
#include "utils/vec.h"
#include "utils/hset.h"


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

struct state *state_alloc(void)
{
    struct state *state = calloc(1, sizeof(*state));

    *state = (struct state) {
        .speed = sim_normal,
        .atoms = atoms_new(),
        .mods = mods_list_reserve(16),
        .chunks = vec64_reserve(16),
        .log = log_new(world_log_cap),
    };
    tech_init(&state->tech);

    return state;
}

void state_free(struct state *state)
{
    atoms_free(state->atoms);
    free(state->mods);
    vec64_free(state->chunks);
    tech_free(&state->tech);
    log_free(state->log);
    htable_reset(&state->names);

    if (state->compile) mod_free(state->compile);
    if (state->mod.mod) mod_free(state->mod.mod);
    if (state->chunk.chunk) chunk_free(state->chunk.chunk);

    for (const struct htable_bucket *it = htable_next(&state->lanes, NULL);
         it; it = htable_next(&state->lanes, it))
    {
        hset_free((void *) it->value);
    }

    free(state);
}


// -----------------------------------------------------------------------------
// save
// -----------------------------------------------------------------------------

static void state_save_chunks(struct world *world, struct save *save)
{
    save_write_magic(save, save_magic_chunks);

    struct chunk *chunk = NULL;
    struct world_chunk_it it = world_chunk_it(world);

    while ((chunk = world_chunk_next(world, &it))) {
        save_write_value(save, coord_to_u64(chunk_star(chunk)->coord));
        save_write_value(save, chunk_name(chunk));
    }
    save_write_value(save, (uint64_t) 0);

    save_write_magic(save, save_magic_chunks);
}

static bool state_load_chunks(struct state *state, struct save *save)
{
    if (!save_read_magic(save, save_magic_chunks)) return false;

    state->chunks->len = 0;
    htable_clear(&state->names);

    while (true) {
        uint64_t coord = save_read_type(save, typeof(coord));
        if (!coord) break;
        word_t name = save_read_type(save, typeof(name));

        state->chunks = vec64_append(state->chunks, coord);
        (void) htable_put(&state->names, coord, name);
    }

    int cmp(const void *lhs_, const void *rhs_) {
        struct coord lhs = coord_from_u64(*((const uint64_t *) lhs_));
        struct coord lhs_sector = coord_sector(lhs);

        struct coord rhs = coord_from_u64(*((const uint64_t *) rhs_));
        struct coord rhs_sector = coord_sector(rhs);

        return coord_eq(lhs_sector, rhs_sector) ?
            coord_cmp(lhs, rhs) : coord_cmp(lhs_sector, rhs_sector);
    }
    vec64_sort_fn(state->chunks, cmp);

    return save_read_magic(save, save_magic_chunks);
}

void state_save(struct save *save, const struct state_ctx *ctx)
{
    save_write_magic(save, save_magic_state_world);
    save_write_value(save, world_seed(ctx->world));
    save_write_value(save, world_time(ctx->world));
    save_write_value(save, ctx->speed);
    save_write_value(save, coord_to_u64(ctx->home));
    save_write_magic(save, save_magic_state_world);

    atoms_save(world_atoms(ctx->world), save);
    mods_list_save(world_mods(ctx->world), save);
    state_save_chunks(ctx->world, save);
    world_lanes_list_save(ctx->world, save);
    tech_save(world_tech(ctx->world), save);
    world_log_save(ctx->world, save);

    {
        save_write_magic(save, save_magic_state_compile);
        if (!ctx->compile) save_write_value(save, (mod_t) 0);
        else {
            save_write_value(save, ctx->compile->id);
            mod_save(ctx->compile, save);
        }
        save_write_magic(save, save_magic_state_compile);
    }

    {
        save_write_magic(save, save_magic_state_mod);
        save_write_value(save, ctx->mod);
        const struct mod *mod = mods_get(world_mods(ctx->world), ctx->mod);
        if (mod) mod_save(mod, save);
        save_write_magic(save, save_magic_state_mod);
    }

    {
        save_write_magic(save, save_magic_state_chunk);
        struct chunk *chunk = world_chunk(ctx->world, ctx->chunk);
        if (!chunk) save_write_value(save, (uint64_t) 0);
        else {
            save_write_value(save, coord_to_u64(ctx->chunk));
            chunk_save(chunk, save);
        }
        save_write_magic(save, save_magic_state_chunk);
    }
}

bool state_load(struct state *state, struct save *save)
{
    if (!save_read_magic(save, save_magic_state_world)) return false;
    save_read_into(save, &state->seed);
    save_read_into(save, &state->time);
    save_read_into(save, &state->speed);
    state->home = coord_from_u64(save_read_type(save, uint64_t));
    if (!save_read_magic(save, save_magic_state_world)) return false;

    if (!atoms_load_into(state->atoms, save)) return false;
    if (!mods_list_load_into(&state->mods, save)) return false;
    if (!state_load_chunks(state, save)) return false;
    if (!lanes_list_load_into(&state->lanes, save)) return false;
    if (!tech_load(&state->tech, save)) return false;
    if (!log_load_into(state->log, save)) return false;

    {
        if (!save_read_magic(save, save_magic_state_compile)) return false;
        if (state->compile) mod_free(state->compile);
        state->compile = NULL;
        mod_t id = save_read_type(save, typeof(id));
        if (id) state->compile = mod_load(save);
        if (!save_read_magic(save, save_magic_state_compile)) return false;
    }

    {
        if (!save_read_magic(save, save_magic_state_mod)) return false;
        if (state->mod.mod) mod_free(state->mod.mod);
        state->mod.mod = NULL;
        save_read_into(save, &state->mod.id);
        if (state->mod.id) state->mod.mod = mod_load(save);
        if (!save_read_magic(save, save_magic_state_mod)) return false;
    }

    {// \todo Need a chunk_load_into; it's not trivial.
        if (!save_read_magic(save, save_magic_state_chunk)) return false;
        if (state->chunk.chunk) chunk_free(state->chunk.chunk);
        state->chunk.chunk = NULL;
        state->chunk.coord = coord_from_u64(save_read_type(save, uint64_t));
        if (!coord_is_nil(state->chunk.coord)) {
            struct chunk *chunk = chunk_load(NULL, save);
            if (!chunk) return false;
            state->chunk.chunk = chunk;
        }
        if (!save_read_magic(save, save_magic_state_chunk)) return false;
    }

    return true;
}
