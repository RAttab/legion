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
#include "utils/symbol.h"

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

void proxy_init(void);
void proxy_free(void);

void proxy_auth(const char *config);

enum proxy_ret { proxy_nil = 0, proxy_updated, proxy_loaded };
enum proxy_ret proxy_update(void);

struct lisp *proxy_lisp(void);
struct lisp_ret proxy_eval(const char *src, size_t len);


// -----------------------------------------------------------------------------
// pipe
// -----------------------------------------------------------------------------

struct proxy_pipe;
struct proxy_pipe *proxy_pipe_new(struct sim_pipe *);
bool proxy_pipe_ready(void);
void proxy_pipe_close(struct proxy_pipe *);
struct save_ring *proxy_pipe_in(struct proxy_pipe *);
struct save_ring *proxy_pipe_out(struct proxy_pipe *);


// -----------------------------------------------------------------------------
// state
// -----------------------------------------------------------------------------

world_seed proxy_seed(void);
world_ts proxy_time(void);
enum speed proxy_speed(void);
struct coord proxy_home(void);

const struct tech *proxy_tech(void);
struct atoms *proxy_atoms(void);
const struct mods_list *proxy_mods(void);
const struct vec64 *proxy_chunks(void);
const struct htable *proxy_lanes(void);
const struct hset *proxy_lanes_for(struct coord star);
const struct log *proxy_logs(void);


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

void proxy_quit(void);
void proxy_save(void);
void proxy_load(void);
void proxy_set_speed(enum speed);
struct chunk *proxy_chunk(struct coord);
void proxy_io(enum io, im_id dst, const vm_word *args, uint8_t len);


// -----------------------------------------------------------------------------
// mod
// -----------------------------------------------------------------------------

const struct mod *proxy_mod(void);
void proxy_mod_select(mod_id);
void proxy_mod_register(struct symbol name);
void proxy_mod_publish(mod_maj);
mod_id proxy_mod_latest(mod_maj);
bool proxy_mod_name(mod_maj, struct symbol *dst);

// transfer ownership of code to proxy.
void proxy_mod_compile(mod_maj, const char *code, size_t len);


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

struct proxy_render_it
{
    struct rect rect;
    struct sector *sector;
    size_t index;
};

struct proxy_render_it proxy_render_it(struct rect viewport);
const struct star *proxy_render_next(struct proxy_render_it *);

bool proxy_active_star(struct coord);
bool proxy_active_sector(struct coord);
struct sector *proxy_sector(struct coord);

vm_word proxy_star_name(struct coord);
const struct star *proxy_star_in(struct rect);
const struct star *proxy_star_at(struct coord);
