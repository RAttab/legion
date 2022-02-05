/* world.h
   Rémi Attab (remi.attab@gmail.com), 30 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/coord.h"
#include "game/user.h"
#include "items/io.h"
#include "items/item.h"
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

typedef uint64_t seed_t;
typedef uint32_t world_ts_t;
typedef int64_t world_ts_delta_t;


// -----------------------------------------------------------------------------
// world
// -----------------------------------------------------------------------------

struct world;

struct world *world_new(seed_t seed);
void world_free(struct world *);

void world_save(struct world *, struct save *);
struct world *world_load(struct save *);

void world_step(struct world *);
void world_populate(struct world *);
void world_populate_user(struct world *, user_t);

seed_t world_seed(struct world *);
world_ts_t world_time(struct world *);
struct mods *world_mods(struct world *);
struct atoms *world_atoms(struct world *);
struct coord world_home(struct world *, user_t);
struct tech *world_tech(struct world *, user_t);
struct chunk *world_chunk(struct world *, struct coord);
struct chunk *world_chunk_alloc(struct world *, struct coord, user_t);
const struct sector *world_sector(struct world *, struct coord);
word_t world_star_name(struct world *, struct coord);
bool world_user_access(struct world *, uset_t, struct coord);

enum { world_log_cap = 64 };
struct log *world_log(struct world *, user_t);
void world_log_push(
        struct world *, user_t, struct coord, id_t, word_t key, word_t value);


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

ssize_t world_scan(struct world *, struct coord, enum item);


// -----------------------------------------------------------------------------
// chunk-it
// -----------------------------------------------------------------------------

struct world_chunk_it
{
    uset_t filter;
    const struct htable_bucket *it;
};

struct vec64 *world_chunk_list(struct world *);
struct world_chunk_it world_chunk_it(struct world *, uset_t);
struct chunk *world_chunk_next(struct world *, struct world_chunk_it *);


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

const struct hset *world_lanes_list(struct world *, struct coord key);
void world_lanes_list_save(struct world *, struct save *, uset_t);

void world_lanes_launch(
        struct world *,
        user_t owner, enum item type, size_t speed,
        struct coord src, struct coord dst,
        const word_t *data, size_t len);
void world_lanes_arrive(
        struct world *,
        user_t owner, enum item type,
        struct coord src, struct coord dst,
        const word_t *data, size_t len);

