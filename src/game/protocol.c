/* protocol.c
   Rémi Attab (remi.attab@gmail.com), 30 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/protocol.h"
#include "game/log.h"
#include "game/chunk.h"
#include "game/save.h"
#include "vm/atoms.h"
#include "vm/mod.h"
#include "utils/vec.h"
#include "utils/hset.h"


// -----------------------------------------------------------------------------
// status
// -----------------------------------------------------------------------------

void status_save(const struct status *status, struct save *save)
{
    save_write_magic(save, save_magic_status);
    save_write_value(save, status->type);
    save_write_value(save, status->len);
    save_write(save, status->msg, status->len);
    save_write_magic(save, save_magic_status);
}

bool status_load(struct status *status, struct save *save)
{
    if (!save_read_magic(save, save_magic_status)) return false;
    save_read_into(save, &status->type);
    save_read_into(save, &status->len);
    save_read(save, status->msg, status->len);
    return save_read_magic(save, save_magic_status);
}


// -----------------------------------------------------------------------------
// ack
// -----------------------------------------------------------------------------

struct ack *ack_new(void)
{
    return calloc(1, sizeof(struct ack));
}

void ack_free(struct ack *ack)
{
    htable_reset((struct htable *) &ack->chunk.provided);
    free(ack);
}

void ack_reset(struct ack *ack)
{
    htable_clear(&ack->chunk.provided);
    memset(ack, 0, sizeof(*ack));
}

void ack_reset_chunk(struct ack *ack)
{
    htable_clear(&ack->chunk.provided);
    memset(&ack->chunk, 0, sizeof(ack->chunk));
}

static void ack_save(const struct ack *ack, struct save *save)
{
    save_write_magic(save, save_magic_ack);

    save_write_value(save, ack->stream);
    save_write_value(save, ack->time);
    save_write_value(save, ack->atoms);

    const struct chunk_ack *cack = &ack->chunk;
    save_write_value(save, coord_to_u64(cack->coord));
    save_write_value(save, cack->time);

    save_write_value(save, (uint8_t) cack->provided.len);
    for (const struct htable_bucket *it = htable_next(&cack->provided, NULL);
         it; it = htable_next(&cack->provided, it))
    {
        save_write_value(save, (enum item) it->key);
        save_write_value(save, it->value);
    }

    save_write_value(save, ring_ack_to_u64(cack->requested));
    save_write_value(save, ring_ack_to_u64(cack->storage));
    save_write_value(save, ring_ack_to_u64(cack->pills));

    for (size_t i = 0; i < array_len(cack->active); ++i) {
        if (!cack->active[i]) continue;
        save_write_value(save, (enum item) (ITEM_ACTIVE_FIRST + i));
        save_write_value(save, cack->active[i]);
    }
    save_write_value(save, (enum item) 0);

    save_write_magic(save, save_magic_ack);
}

static struct ack *ack_load(struct save *save)
{
    if (!save_read_magic(save, save_magic_ack)) return NULL;
    struct ack *ack = ack_new();

    save_read_into(save, &ack->stream);
    save_read_into(save, &ack->time);
    save_read_into(save, &ack->atoms);

    struct chunk_ack *cack = &ack->chunk;
    cack->coord = coord_from_u64(save_read_type(save, uint64_t));
    save_read_into(save, &cack->time);

    size_t len = save_read_type(save, uint8_t);
    htable_clear(&cack->provided);
    htable_reserve(&cack->provided, len);
    for (size_t i = 0; i < len; ++i) {
        enum item key = save_read_type(save, typeof(key));
        uint64_t value = save_read_type(save, typeof(value));

        struct htable_ret ret = htable_put(&cack->provided, key, value);
        assert(ret.ok);
    }

    cack->requested = ring_ack_from_u64(save_read_type(save, uint64_t));
    cack->storage = ring_ack_from_u64(save_read_type(save, uint64_t));
    cack->pills = ring_ack_from_u64(save_read_type(save, uint64_t));

    memset(cack->active, 0, sizeof(cack->active));
    while (true) {
        enum item item = save_read_type(save, typeof(item));
        if (!item) break;

        hash_t hash = save_read_type(save, typeof(hash));
        cack->active[item - ITEM_ACTIVE_FIRST] = hash;
    }

    if (!save_read_magic(save, save_magic_ack)) { free(ack); return NULL; }
    return ack;
}

// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

void cmd_save(const struct cmd *cmd, struct save *save)
{
    save_write_magic(save, save_magic_cmd);
    save_write_value(save, cmd->type);

    switch (cmd->type)
    {

    case CMD_NIL:
    case CMD_QUIT: { break; }

    case CMD_SAVE:
    case CMD_LOAD: { break; }

    case CMD_USER: {
        save_write_value(save, cmd->data.user.server);
        symbol_save(&cmd->data.user.name, save);
        break;
    }

    case CMD_AUTH: {
        save_write_value(save, cmd->data.auth.server);
        save_write_value(save, cmd->data.auth.id);
        save_write_value(save, cmd->data.auth.private);
        break;
    }

    case CMD_ACK: {
        ack_save(cmd->data.ack, save);
        break;
    }

    case CMD_SPEED: {
        save_write_value(save, cmd->data.speed);
        break;
    }

    case CMD_CHUNK: {
        save_write_value(save, coord_to_u64(cmd->data.chunk));
        break;
    }

    case CMD_MOD: {
        save_write_value(save, cmd->data.mod);
        break;
    }

    case CMD_MOD_REGISTER: {
        symbol_save(&cmd->data.mod_register, save);
        break;
    }

    case CMD_MOD_PUBLISH: {
        save_write_value(save, cmd->data.mod_publish.maj);
        break;
    }

    case CMD_MOD_COMPILE: {
        save_write_value(save, cmd->data.mod_compile.maj);
        save_write_value(save, cmd->data.mod_compile.len);
        save_write(save, cmd->data.mod_compile.code, cmd->data.mod_compile.len);
        break;
    }

    case CMD_IO: {
        save_write_value(save, cmd->data.io.io);
        save_write_value(save, cmd->data.io.dst);
        save_write_value(save, cmd->data.io.len);
        save_write(save, cmd->data.io.args,
                cmd->data.io.len * sizeof(cmd->data.io.args[0]));
        break;
    }

    default: { assert(false); }
    }

    save_write_magic(save, save_magic_cmd);
}

bool cmd_load(struct cmd *cmd, struct save *save)
{
    if (!save_read_magic(save, save_magic_cmd)) return false;
    save_read_into(save, &cmd->type);

    switch (cmd->type)
    {

    case CMD_NIL:
    case CMD_QUIT: { break; }

    case CMD_SAVE:
    case CMD_LOAD: { break; }

    case CMD_USER: {
        save_read_into(save, &cmd->data.user.server);
        symbol_load(&cmd->data.user.name, save);
        break;
    }

    case CMD_AUTH: {
        save_read_into(save, &cmd->data.auth.server);
        save_read_into(save, &cmd->data.auth.id);
        save_read_into(save, &cmd->data.auth.private);
        break;
    }

    case CMD_ACK: {
        if (!(cmd->data.ack = ack_load(save))) return false;
        break;
    }

    case CMD_SPEED: {
        save_read_into(save, &cmd->data.speed);
        break;
    }

    case CMD_CHUNK: {
        cmd->data.chunk = coord_from_u64(save_read_type(save, uint64_t));
        break;
    }

    case CMD_MOD: {
        save_read_into(save, &cmd->data.mod);
        break;
    }

    case CMD_MOD_REGISTER: {
        if (!symbol_load(&cmd->data.mod_register, save)) return false;
        break;
    }

    case CMD_MOD_PUBLISH: {
        save_read_into(save, &cmd->data.mod_publish.maj);
        break;
    }

    case CMD_MOD_COMPILE: {
        save_read_into(save, &cmd->data.mod_compile.maj);
        save_read_into(save, &cmd->data.mod_compile.len);

        char *code = calloc(1, cmd->data.mod_compile.len);
        save_read(save, code, cmd->data.mod_compile.len);
        cmd->data.mod_compile.code = code;
        break;
    }

    case CMD_IO: {
        save_read_into(save, &cmd->data.io.io);
        save_read_into(save, &cmd->data.io.dst);
        save_read_into(save, &cmd->data.io.len);
        save_read(save, cmd->data.io.args,
                cmd->data.io.len * sizeof(cmd->data.io.args[0]));
        break;
    }

    default: { assert(false); }
    }

    return save_read_magic(save, save_magic_cmd);
}


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

struct state *state_alloc(void)
{
    struct state *state = calloc(1, sizeof(*state));

    *state = (struct state) {
        .speed = speed_slow,
        .atoms = atoms_new(),
        .mods = mods_list_reserve(16),
        .chunks = vec64_reserve(16),
        .log = log_new(world_log_cap),
        .chunk = { .chunk = chunk_alloc_empty() },
    };

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

static void state_save_chunks(
        struct world *world,
        struct save *save,
        const struct state_ctx *ctx)
{
    save_write_magic(save, save_magic_chunks);

    struct chunk *chunk = NULL;
    struct world_chunk_it it = world_chunk_it(world, ctx->access);

    while ((chunk = world_chunk_next(world, &it))) {
        if (chunk_updated(chunk) < ctx->ack->time) continue;
        save_write_value(save, coord_to_u64(chunk_star(chunk)->coord));
        save_write_value(save, chunk_name(chunk));
    }
    save_write_value(save, (uint64_t) 0);

    save_write_magic(save, save_magic_chunks);
}

static bool state_load_chunks(struct state *state, struct save *save)
{
    if (!save_read_magic(save, save_magic_chunks)) return false;

    while (true) {
        uint64_t coord = save_read_type(save, typeof(coord));
        if (!coord) break;
        word_t name = save_read_type(save, typeof(name));

        struct htable_ret ret = htable_put(&state->names, coord, name);
        if (ret.ok) state->chunks = vec64_append(state->chunks, coord);
        else {
            ret = htable_xchg(&state->names, coord, name);
            assert(ret.ok);
        }
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
    save_write_value(save, ctx->stream);
    save_write_value(save, world_seed(ctx->world));
    save_write_value(save, world_time(ctx->world));
    save_write_value(save, ctx->speed);
    save_write_value(save, coord_to_u64(world_home(ctx->world, ctx->user)));
    save_write_magic(save, save_magic_state_world);

    atoms_save_delta(world_atoms(ctx->world), save, ctx->ack);
    mods_list_save(world_mods(ctx->world), save, ctx->access);
    state_save_chunks(ctx->world, save, ctx);
    world_lanes_list_save(ctx->world, save, ctx->access);
    tech_save(world_tech(ctx->world, ctx->user), save);
    log_save_delta(world_log(ctx->world, ctx->user), save, ctx->ack->time);

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
            chunk_save_delta(chunk, save, ctx->ack);
        }
        save_write_magic(save, save_magic_state_chunk);
    }
}

bool state_load(struct state *state, struct save *save, struct ack *ack)
{
    if (!save_read_magic(save, save_magic_state_world)) return false;
    save_read_into(save, &state->stream);
    save_read_into(save, &state->seed);
    save_read_into(save, &state->time);
    save_read_into(save, &state->speed);
    state->home = coord_from_u64(save_read_type(save, uint64_t));
    if (!save_read_magic(save, save_magic_state_world)) return false;

    // States that are strictly increasing, we need to clear them out manually
    // on load as there's no ack information that we can use.
    if (state->stream != ack->stream) {
        ack_reset(ack);
        state->mods->len = 0;
        state->chunks->len = 0;
        htable_clear(&state->names);
    }

    if (!atoms_load_delta(state->atoms, save, ack)) return false;
    if (!mods_list_load_into(&state->mods, save)) return false;
    if (!state_load_chunks(state, save)) return false;
    if (!lanes_list_load_into(&state->lanes, save)) return false;
    if (!tech_load(&state->tech, save)) return false;
    if (!log_load_delta(state->log, save, ack->time)) return false;

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

    {
        if (!save_read_magic(save, save_magic_state_chunk)) return false;
        state->chunk.coord = coord_from_u64(save_read_type(save, uint64_t));
        if (!coord_is_nil(state->chunk.coord)) {
            if (!chunk_load_delta(state->chunk.chunk, save, ack)) return false;
        }
        if (!save_read_magic(save, save_magic_state_chunk)) return false;
    }

    ack->stream = state->stream;
    ack->time = ack->chunk.time = state->time;
    return true;
}