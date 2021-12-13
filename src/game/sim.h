/* sim.h
   Rémi Attab (remi.attab@gmail.com), 28 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "game/coord.h"
#include "game/protocol.h"
#include "game/world.h"
#include "vm/symbol.h"
#include "vm/mod.h"
#include "vm/vm.h"
#include "items/io.h"

struct save;


// -----------------------------------------------------------------------------
// sim
// -----------------------------------------------------------------------------

struct sim;

struct sim *sim_new(seed_t seed);
void sim_close(struct sim *);

struct cmd *sim_cmd_write(struct sim *);
void sim_cmd_push(struct sim *);

struct save *sim_state_read(struct sim *);
void sim_state_release(struct sim *, struct save *);


struct sim_log
{
    enum status type;
    size_t len;
    char msg[256];
};

void sim_logv(struct sim *, enum status, const char *fmt, va_list);
void sim_log(struct sim *, enum status, const char *fmt, ...) legion_printf(3, 4);
const struct sim_log *sim_log_read(struct sim *);
void sim_log_pop(struct sim *);
