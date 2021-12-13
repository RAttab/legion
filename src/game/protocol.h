/* protocol.h
   Rémi Attab (remi.attab@gmail.com), 30 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/coord.h"
#include "game/world.h"
#include "game/chunk.h"
#include "game/tech.h"
#include "utils/htable.h"

struct vec64;
struct log;
struct atoms;
struct mods_list;
struct mod;
struct save;


// -----------------------------------------------------------------------------
// status
// -----------------------------------------------------------------------------

enum status
{
    st_info = 0,
    st_warn = 1,
    st_error = 2,
};


// -----------------------------------------------------------------------------
// speed
// -----------------------------------------------------------------------------

enum legion_packed speed
{
    speed_pause = 0,
    speed_normal,
    speed_fast,
};

static_assert(sizeof(enum speed) == 1);


// -----------------------------------------------------------------------------
// ack
// -----------------------------------------------------------------------------

struct chunk_ack
{
    struct coord coord;
    world_ts_t time;

    struct htable provided;
    struct ring_ack requested;
    struct ring_ack storage;

    struct ring_ack pills;
    hash_t active[ITEMS_ACTIVE_LEN];
};

struct ack
{
    uint64_t stream;
    world_ts_t time;
    uint32_t atoms;
    struct chunk_ack chunk;
};

const struct ack *ack_clone(const struct ack *);
void ack_free(const struct ack *);

void ack_reset(struct ack *);
void ack_reset_chunk(struct ack *);


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

enum cmd_type
{
    CMD_NIL = 0,
    CMD_QUIT,

    CMD_SAVE,
    CMD_LOAD,

    CMD_ACK,
    CMD_SPEED,
    CMD_CHUNK,

    CMD_MOD,
    CMD_MOD_REGISTER,
    CMD_MOD_PUBLISH,
    CMD_MOD_COMPILE,

    CMD_IO,
};

struct cmd
{
    enum cmd_type type;

    union
    {
        const struct ack *ack;
        enum speed speed;
        struct coord chunk;
        mod_t mod;
        struct symbol mod_register;
        struct { mod_maj_t maj; } mod_publish;
        struct { mod_maj_t maj; const char *code; uint32_t len; } mod_compile;
        struct { enum io io; id_t dst; uint8_t len; word_t args[4]; } io;
    } data;
};

void cmd_save(const struct cmd *cmd, struct save *);
bool cmd_load(struct cmd *cmd, struct save *);


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

struct state
{
    uint64_t stream;

    seed_t seed;
    world_ts_t time;
    enum speed speed;
    struct coord home;

    struct atoms *atoms;
    struct mods_list *mods;
    struct vec64 *chunks;
    struct htable lanes;
    struct htable names;
    struct tech tech;
    struct log *log;
    const struct mod *compile;

    struct
    {
        mod_t id;
        const struct mod *mod;
    } mod;

    struct
    {
        struct coord coord;
        struct chunk *chunk;
    } chunk;

};

struct state *state_alloc(void);
void state_free(struct state *);


struct state_ctx
{
    uint64_t stream;

    struct world *world;
    enum speed speed;
    struct coord home;

    mod_t mod;
    struct coord chunk;
    const struct mod *compile;

    const struct ack *ack;
};

void state_save(struct save *, const struct state_ctx *);
bool state_load(struct state *, struct save *, struct ack *);
