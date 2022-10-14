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

enum proxy_ret { proxy_nil = 0, proxy_updated, proxy_loaded };
enum proxy_ret proxy_update(struct proxy *);

struct lisp *proxy_lisp(struct proxy *);
struct lisp_ret proxy_eval(struct proxy *, const char *src, size_t len);


// -----------------------------------------------------------------------------
// pipe
// -----------------------------------------------------------------------------

struct proxy_pipe;
struct proxy_pipe *proxy_pipe_new(struct proxy *, struct sim_pipe *);
bool proxy_pipe_ready(struct proxy *);
void proxy_pipe_close(struct proxy_pipe *);
struct save_ring *proxy_pipe_in(struct proxy_pipe *);
struct save_ring *proxy_pipe_out(struct proxy_pipe *);

// -----------------------------------------------------------------------------
// notify
// -----------------------------------------------------------------------------
// Unlike EV_STATE_UPDATE, the fn provided will be invoked for sim step that
// wasn't skipped over. Use sparringly.

typedef void (*proxy_fn) (void *);
void proxy_notify(struct proxy *, proxy_fn, void *);

// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

world_seed proxy_seed(struct proxy *);
world_ts proxy_time(struct proxy *);
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
void proxy_io(struct proxy *, enum io, im_id dst, const vm_word *args, uint8_t len);


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

const struct mod *proxy_mod(struct proxy *);
void proxy_mod_select(struct proxy *, mod_id);
void proxy_mod_register(struct proxy *, struct symbol name);
void proxy_mod_publish(struct proxy *, mod_maj);
mod_id proxy_mod_latest(struct proxy *, mod_maj);
bool proxy_mod_name(struct proxy *, mod_maj, struct symbol *dst);

// transfer ownership of code to proxy.
void proxy_mod_compile(struct proxy *, mod_maj, const char *code, size_t len);


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

vm_word proxy_star_name(struct proxy *, struct coord);
const struct star *proxy_star_in(struct proxy *, struct rect);
const struct star *proxy_star_at(struct proxy *, struct coord);
