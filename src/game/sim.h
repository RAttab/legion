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
void sim_free(struct sim *);

struct save_ring *sim_in(struct sim *);
struct save_ring *sim_out(struct sim *);

void sim_step(struct sim *);
void sim_loop(struct sim *);

void sim_thread(struct sim *);
void sim_quit(struct sim *);

void sim_logv(struct sim *, enum status_type, const char *fmt, va_list);
void sim_log(struct sim *, enum status_type, const char *fmt, ...)
    legion_printf(3, 4);
