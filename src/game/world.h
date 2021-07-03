/* world.h
   Rémi Attab (remi.attab@gmail.com), 30 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/save.h"
#include "game/coord.h"
#include "game/sector.h"

struct mods;
typedef uint32_t world_ts_t;

// -----------------------------------------------------------------------------
// world
// -----------------------------------------------------------------------------

struct world;

struct world *world_new(void);
void world_free(struct world *);

struct world *world_load(struct save *);
void world_save(struct world *, struct save *);

void world_step(struct world *);
struct coord world_populate(struct world *);

world_ts_t world_time(struct world *);
struct mods *world_mods(struct world *);
struct chunk *world_chunk(struct world *, struct coord);
struct sector *world_sector(struct world *, struct coord);
const struct star *world_star(struct world *, struct rect);


struct world_render_it
{
    struct rect rect;
    struct sector *sector;
    size_t index;
};

struct world_render_it world_render_it(struct world *, struct rect viewport);
const struct star *world_render_next(struct world *, struct world_render_it *);


struct legion_packed world_scan_it
{
    struct coord coord;
    uint64_t index;
};

struct world_scan_it world_scan_it(struct world *, struct coord coord);
struct coord world_scan_next(struct world *, struct world_scan_it *);

ssize_t world_scan(struct world *, struct coord, item_t);

void world_lanes_launch(struct world *,
        struct coord src, struct coord dst,
        item_t type, item_t cargo, uint8_t count);
void world_lanes_arrive(struct world *,
        struct coord dst, item_t type, item_t cargo, uint8_t count);
