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

enum
{
    sim_in_len = 100 * s_page_len,
    sim_out_len = 2000 * s_page_len,
};

struct sim *sim_new(seed_t seed);
struct sim *sim_load(void);
void sim_free(struct sim *);

struct sim_pipe;
struct sim_pipe *sim_pipe_new(struct sim *);
void sim_pipe_close(struct sim_pipe *);
struct save_ring *sim_pipe_in(struct sim_pipe *);
struct save_ring *sim_pipe_out(struct sim_pipe *);

void sim_step(struct sim *);
void sim_loop(struct sim *);

void sim_thread(struct sim *);
void sim_join(struct sim *);
