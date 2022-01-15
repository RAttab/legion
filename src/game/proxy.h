/* proxy.h
   Rémi Attab (remi.attab@gmail.com), 29 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/coord.h"
#include "game/sim.h"
#include "vm/vm.h"
#include "vm/mod.h"
#include "vm/symbol.h"

struct sector;
struct tech;
struct log;
struct atoms;
struct mods_list;
struct vec64;
struct htable;
struct hset;


// -----------------------------------------------------------------------------
// proxy
// -----------------------------------------------------------------------------

struct proxy;

struct proxy *proxy_new(void);
void proxy_free(struct proxy *);

void proxy_auth(struct proxy *, const char *config);
bool proxy_update(struct proxy *);

struct lisp_ret proxy_eval(struct proxy *, const char *src, size_t len);


// -----------------------------------------------------------------------------
// pipe
// -----------------------------------------------------------------------------

struct proxy_pipe;
struct proxy_pipe *proxy_pipe_new(struct proxy *, struct sim_pipe *);
void proxy_pipe_close(struct proxy *, struct proxy_pipe *);
struct save_ring *proxy_pipe_in(struct proxy_pipe *);
struct save_ring *proxy_pipe_out(struct proxy_pipe *);


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

seed_t proxy_seed(struct proxy *);
world_ts_t proxy_time(struct proxy *);
enum speed proxy_speed(struct proxy *);
struct coord proxy_home(struct proxy *);

const struct tech *proxy_tech(struct proxy *);
struct atoms *proxy_atoms(struct proxy *);
const struct mods_list *proxy_mods(struct proxy *);
const struct vec64 *proxy_chunks(struct proxy *);
const struct htable *proxy_lanes(struct proxy *);
const struct hset *proxy_lanes_for(struct proxy *, struct coord star);
const struct log *proxy_logs(struct proxy *);


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

void proxy_quit(struct proxy *);
void proxy_save(struct proxy *);
void proxy_load(struct proxy *);
void proxy_set_speed(struct proxy *, enum speed);
struct chunk *proxy_chunk(struct proxy *, struct coord);
void proxy_io(struct proxy *, enum io, id_t dst, const word_t *args, uint8_t len);


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

mod_t proxy_mod_id(struct proxy *);
const struct mod *proxy_mod(struct proxy *, mod_t);
void proxy_mod_register(struct proxy *, struct symbol name);
void proxy_mod_publish(struct proxy *, mod_maj_t);
mod_t proxy_mod_latest(struct proxy *, mod_maj_t);
bool proxy_mod_name(struct proxy *, mod_maj_t, struct symbol *dst);

// transfer ownership of code to proxy.
void proxy_mod_compile(struct proxy *, mod_maj_t, const char *code, size_t len);
const struct mod *proxy_mod_compile_result(struct proxy *);


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

struct proxy_render_it
{
    struct rect rect;
    struct sector *sector;
    size_t index;
};

struct proxy_render_it proxy_render_it(struct proxy *, struct rect viewport);
const struct star *proxy_render_next(struct proxy *, struct proxy_render_it *);

bool proxy_active_star(struct proxy *, struct coord);
bool proxy_active_sector(struct proxy *, struct coord);
struct sector *proxy_sector(struct proxy *, struct coord);

word_t proxy_star_name(struct proxy *, struct coord);
const struct star *proxy_star_in(struct proxy *, struct rect);
const struct star *proxy_star_at(struct proxy *, struct coord);
