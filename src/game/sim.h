/* sim.h
   Rémi Attab (remi.attab@gmail.com), 28 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "game/coord.h"
#include "game/state.h"
#include "game/world.h"
#include "vm/symbol.h"
#include "vm/mod.h"
#include "vm/vm.h"
#include "items/io.h"

struct save;


// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

enum status
{
    st_info = 0,
    st_warn = 1,
    st_error = 2,
};

struct sim_log
{
    enum status type;
    size_t len;
    char msg[256];
};


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

enum sim_cmd_type
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
    CMD_MOD_COMPILE,
    CMD_MOD_PUBLISH,

    CMD_IO,
};

struct sim_cmd
{
    enum sim_cmd_type type;

    union
    {
        const struct ack *ack;
        enum speed speed;
        struct coord chunk;
        mod_t mod;
        struct symbol mod_register;
        struct { mod_maj_t maj; } mod_publish;
        struct { mod_maj_t maj; const char *code; size_t len; } mod_compile;
        struct { enum io io; id_t dst; uint8_t len; word_t args[4]; } io;
    } data;
};


// -----------------------------------------------------------------------------
// sim
// -----------------------------------------------------------------------------

struct sim;

struct sim *sim_new(seed_t seed);
void sim_close(struct sim *);

void sim_logv(struct sim *, enum status, const char *fmt, va_list);
void sim_log(struct sim *, enum status, const char *fmt, ...) legion_printf(3, 4);
const struct sim_log *sim_log_read(struct sim *);
void sim_log_pop(struct sim *);

struct sim_cmd *sim_cmd_write(struct sim *);
void sim_cmd_push(struct sim *);

struct save *sim_state_read(struct sim *);
void sim_state_release(struct sim *, struct save *);
