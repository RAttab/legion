/* world.h
   Rémi Attab (remi.attab@gmail.com), 30 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

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
// world
// -----------------------------------------------------------------------------

struct world;

struct world *world_new(world_seed seed, struct metrics *);
void world_free(struct world *);

void world_save(struct world *, struct save *);
struct world *world_load(struct save *);

void world_step(struct world *);
void world_populate(struct world *);
void world_populate_user(struct world *, user_id);

world_seed world_gen_seed(const struct world *);
world_ts world_time(const struct world *);
struct mods *world_mods(struct world *);
struct atoms *world_atoms(struct world *);
struct coord world_home(struct world *, user_id);
struct tech *world_tech(struct world *, user_id);
struct chunk *world_chunk(struct world *, struct coord);
struct chunk *world_chunk_alloc(struct world *, struct coord, user_id);
const struct sector *world_sector(struct world *, struct coord);
vm_word world_star_name(struct world *, struct coord);
bool world_user_access(struct world *, user_set, struct coord);
struct metrics *world_metrics(struct world *);


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

enum : size_t { world_log_cap = 64 };
struct log *world_log(struct world *, user_id);


// -----------------------------------------------------------------------------
// user-io
// -----------------------------------------------------------------------------

struct user_io *world_user_io(struct world *, user_id);
void world_user_io_clear(struct world *, user_id);


// -----------------------------------------------------------------------------
// scan / probe
// -----------------------------------------------------------------------------

ssize_t world_probe(struct world *, struct coord, enum item);
struct coord world_scan(struct world *, struct scan_it);


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

struct lanes *world_lanes(struct world *);

void world_lanes_arrive(
        struct world *,
        user_id owner, enum item type,
        struct coord src, struct coord dst,
        const vm_word *data, size_t len);

