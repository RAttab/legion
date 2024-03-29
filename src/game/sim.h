/* sim.h
   Rémi Attab (remi.attab@gmail.com), 28 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct save;


// -----------------------------------------------------------------------------
// sim
// -----------------------------------------------------------------------------

struct sim;

enum : size_t
{
    sim_in_len = 100 * sys_page_len,
    sim_out_len = 2000 * sys_page_len,
};

struct sim *sim_new(world_seed seed, const char *save);
void sim_free(struct sim *);

void sim_server(struct sim *, const char *config);
void sim_server_reload(struct sim *);

struct sim_pipe;
struct sim_pipe *sim_pipe_new(struct sim *, sys_ts publish_period);
void sim_pipe_close(struct sim_pipe *);
struct save_ring *sim_pipe_in(struct sim_pipe *);
struct save_ring *sim_pipe_out(struct sim_pipe *);

void sim_save(struct sim *);
void sim_load(struct sim *);

void sim_step(struct sim *);
void sim_loop(struct sim *);

void sim_fork(struct sim *);
void sim_join(struct sim *);
