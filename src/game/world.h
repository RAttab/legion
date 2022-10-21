/* world.h
   Rémi Attab (remi.attab@gmail.com), 30 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "game/coord.h"
#include "game/user.h"
#include "db/io.h"
#include "db/items.h"
#include "vm/mod.h"
#include "vm/vm.h"

struct mods;
struct hset;
struct logi;
struct tech;
struct atoms;
struct chunk;
struct sector;
struct save;
struct htable_bucket;


// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

typedef uint64_t world_seed;
typedef uint32_t world_ts;
typedef int64_t world_ts_delta;


// -----------------------------------------------------------------------------
// world
// -----------------------------------------------------------------------------

struct world;

struct world *world_new(world_seed seed);
void world_free(struct world *);

void world_save(struct world *, struct save *);
struct world *world_load(struct save *);

void world_step(struct world *);
void world_populate(struct world *);
void world_populate_user(struct world *, user_id);

world_seed world_gen_seed(struct world *);
world_ts world_time(struct world *);
struct mods *world_mods(struct world *);
struct atoms *world_atoms(struct world *);
struct coord world_home(struct world *, user_id);
struct tech *world_tech(struct world *, user_id);
struct chunk *world_chunk(struct world *, struct coord);
struct chunk *world_chunk_alloc(struct world *, struct coord, user_id);
const struct sector *world_sector(struct world *, struct coord);
vm_word world_star_name(struct world *, struct coord);
bool world_user_access(struct world *, user_set, struct coord);

enum { world_log_cap = 64 };
struct log *world_log(struct world *, user_id);
void world_log_push(
        struct world *, user_id, struct coord, im_id, vm_word key, vm_word value);

struct world_io { enum io io; id_t src; vm_word args[4]; uint8_t len; };
struct world_io *world_user_io(struct world *, user_id);
void world_user_io_clear(struct world *, user_id);

// -----------------------------------------------------------------------------
// scan-it
// -----------------------------------------------------------------------------

struct legion_packed world_scan_it
{
    struct coord coord;
    uint64_t index;
};

struct world_scan_it world_scan_it(struct world *, struct coord coord);
struct coord world_scan_next(struct world *, struct world_scan_it *);
struct coord world_scan_peek(struct world *, const struct world_scan_it *);

ssize_t world_scan(struct world *, struct coord, enum item);


// -----------------------------------------------------------------------------
// chunk-it
// -----------------------------------------------------------------------------

struct world_chunk_it
{
    user_set filter;
    const struct htable_bucket *it;
};

struct vec64 *world_chunk_list(struct world *);
struct world_chunk_it world_chunk_it(struct world *, user_set);
struct chunk *world_chunk_next(struct world *, struct world_chunk_it *);


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

const struct hset *world_lanes_list(struct world *, struct coord key);
void world_lanes_list_save(struct world *, struct save *, user_set);

void world_lanes_launch(
        struct world *,
        user_id owner, enum item type, size_t speed,
        struct coord src, struct coord dst,
        const vm_word *data, size_t len);
void world_lanes_arrive(
        struct world *,
        user_id owner, enum item type,
        struct coord src, struct coord dst,
        const vm_word *data, size_t len);

