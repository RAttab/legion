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
#include "game/user.h"
#include "utils/htable.h"

struct vec64;
struct log;
struct atoms;
struct mods_list;
struct mod;
struct save;


// -----------------------------------------------------------------------------
// speed
// -----------------------------------------------------------------------------

enum legion_packed speed
{
    speed_pause = 0,
    speed_slow,
    speed_fast,
    speed_faster,
    speed_fastest,
};

static_assert(sizeof(enum speed) == 1);


// -----------------------------------------------------------------------------
// header
// -----------------------------------------------------------------------------

enum legion_packed { header_magic = 0xF0FCCF0FU };
static_assert(sizeof(header_magic) == 4);

enum legion_packed header_type
{
    header_nil = 0,
    header_cmd = 1,
    header_state = 2,
    header_status = 3,
    header_user = 4,
    header_mod = 5,
};

static_assert(sizeof(enum header_type) == 1);


struct legion_packed header
{
    uint32_t magic:32;
    uint32_t type:8;
    uint32_t len:24;
};

static_assert(sizeof(struct header) == sizeof(uint64_t));


inline struct header make_header(enum header_type type, size_t len)
{
    return (struct header) {
        .magic = header_magic,
        .type = type,
        .len = len,
    };
}


// -----------------------------------------------------------------------------
// status
// -----------------------------------------------------------------------------

enum legion_packed status_type
{
    st_info = 0,
    st_warn = 1,
    st_error = 2,
};

static_assert(sizeof(enum status_type) == 1);


struct legion_packed status
{
    enum status_type type;
    uint8_t len;
    char msg[126];
};

static_assert(sizeof(struct status) == 128);


void status_save(const struct status *, struct save *);
bool status_load(struct status *, struct save *);


// -----------------------------------------------------------------------------
// ack
// -----------------------------------------------------------------------------

struct chunk_ack
{
    struct coord coord;
    world_ts time;

    struct htable provided;
    struct ring_ack requested;
    struct ring_ack storage;

    struct ring_ack pills;
    hash active[ITEMS_ACTIVE_LEN];
};


struct ack
{
    uint64_t stream;
    world_ts time;
    uint32_t atoms;
    struct chunk_ack chunk;
};


struct ack *ack_new(void);
void ack_free(struct ack *);
void ack_reset(struct ack *);
void ack_reset_chunk(struct ack *);


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

enum cmd_type
{
    CMD_NIL          = 0x00,
    CMD_QUIT         = 0x01,

    CMD_SAVE         = 0x10,
    CMD_LOAD         = 0x11,
    CMD_USER         = 0x12,
    CMD_AUTH         = 0x13,

    CMD_ACK          = 0x20,
    CMD_SPEED        = 0x21,
    CMD_CHUNK        = 0x22,

    CMD_MOD          = 0x30,
    CMD_MOD_REGISTER = 0x31,
    CMD_MOD_PUBLISH  = 0x32,
    CMD_MOD_COMPILE  = 0x33,

    CMD_IO           = 0x40,
};


struct cmd
{
    enum cmd_type type;

    union
    {
        struct { token server; struct symbol name; } user;
        struct { token server; user id; token private; } auth;

        struct ack *ack;
        enum speed speed;
        struct coord chunk;
        mod_id mod;
        struct symbol mod_register;
        struct { mod_maj maj; } mod_publish;
        struct { mod_maj maj; const char *code; uint32_t len; } mod_compile;
        struct { enum io io; id dst; uint8_t len; word args[4]; } io;
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

    seed seed;
    world_ts time;
    enum speed speed;
    struct coord home;

    struct atoms *atoms;
    struct mods_list *mods;
    struct vec64 *chunks;
    struct htable lanes;
    struct htable names;
    struct tech tech;
    struct log *log;

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
    uset access;
    user user;

    struct world *world;
    enum speed speed;
    struct coord chunk;

    const struct ack *ack;
};

void state_save(struct save *, const struct state_ctx *);
bool state_load(struct state *, struct save *, struct ack *);
