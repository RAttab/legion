/* world.h
   Rémi Attab (remi.attab@gmail.com), 30 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/save.h"
#include "game/coord.h"
#include "game/sector.h"
#include "items/io.h"
#include "vm/mod.h"

struct mods;
struct hset;
struct logi;

typedef uint64_t seed_t;
typedef uint32_t world_ts_t;
typedef int64_t world_ts_delta_t;


// -----------------------------------------------------------------------------
// world
// -----------------------------------------------------------------------------

struct world;

struct world *world_new(seed_t seed);
void world_free(struct world *);

struct world *world_load(struct save *);
void world_save(struct world *, struct save *);

void world_step(struct world *);
struct coord world_populate(struct world *);

seed_t world_seed(struct world *);
world_ts_t world_time(struct world *);
struct mods *world_mods(struct world *);
struct atoms *world_atoms(struct world *);
struct chunk *world_chunk(struct world *, struct coord);
struct sector *world_sector(struct world *, struct coord);
const struct star *world_star_in(struct world *, struct rect);
const struct star *world_star_at(struct world *, struct coord);
word_t world_star_name(struct world *, struct coord);

struct lisp_ret world_eval(struct world *, const char *src, size_t len);


// -----------------------------------------------------------------------------
// render-it
// -----------------------------------------------------------------------------

struct world_render_it
{
    struct rect rect;
    struct sector *sector;
    size_t index;
};

struct world_render_it world_render_it(struct world *, struct rect viewport);
const struct star *world_render_next(struct world *, struct world_render_it *);


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
    const struct htable_bucket *sector;
    const struct htable_bucket *chunk;
};

struct world_chunk_it world_chunk_it(struct world *);
struct coord world_chunk_next(struct world *, struct world_chunk_it *);


// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

enum { world_log_cap = 32 };
void world_log(struct world *, struct coord, id_t, enum io, enum ioe);
const struct logi *world_log_next(struct world *, const struct logi *it);


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

const struct hset *world_lanes_list(struct world *, struct coord key);
void world_lanes_launch(
        struct world *,
        enum item type, size_t speed,
        struct coord src, struct coord dst,
        const word_t *data, size_t len);
void world_lanes_arrive(
        struct world *,
        enum item type,
        struct coord src, struct coord dst,
        const word_t *data, size_t len);


// -----------------------------------------------------------------------------
// lab
// -----------------------------------------------------------------------------

bool world_lab_known(struct world *, enum item);
struct tape_set world_lab_known_list(struct world *);

void world_lab_learn(struct world *, enum item);
bool world_lab_learned(struct world *, enum item);
struct tape_set world_lab_learned_list(struct world *);

uint64_t world_lab_learned_bits(struct world *, enum item);
void world_lab_learn_bit(struct world *, enum item, uint8_t bit);
