/* world.c
   Rémi Attab (remi.attab@gmail.com), 30 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/world.h"


// -----------------------------------------------------------------------------
// world
// -----------------------------------------------------------------------------


struct world
{
    struct htable sectors;
};

struct world *world_new(void)
{
    struct world *world = calloc(1, sizeof(*world));
    htable_reset(&world->sectors);
    return world;
}

void world_free(struct world *world)
{
    struct htable_bucket *it = htable_next(&world->sectors, NULL);
    for (; it; it = htable_next(&world->sectors, it))
        sector_free((struct sector *) it->value);
    htable_reset(&world->sectors);
    free(world);
}

void world_step(struct world *world)
{
    struct htable_bucket *it = htable_next(&world->sectors, NULL);
    for (; it; it = htable_next(&world->sectors, it))
        sector_step((struct sector *) it->value);
}

struct coord world_populate(struct world *world)
{
    struct rng rng = rng_make(0);

    while (true) {
        struct coord coord = id_to_coord(rng_step(&rng));
        struct sector *sector = world_sector(world, coord);

        struct star *star = NULL;
        for (size_t tries = 0; tries < 10; ++tries) {
            size_t index = rng_uni(&rng, 0, sector->stars_len);
            star = &sector->stars[index];

            size_t sum = 0;
            for (size_t i = 0; i < ITEMS_NATURAL_LEN; ++i) sum += star->elems[i];
            if (sum < 1 << 16) continue;

            struct chunk *chunk = sector_chunk_alloc(sector, star->coord);
            chunk_create(chunk, ITEM_MINER);
            chunk_create(chunk, ITEM_MINER);
            chunk_create(chunk, ITEM_PRINTER);
            chunk_create(chunk, ITEM_WORKER);
            chunk_create(chunk, ITEM_DEPLOYER);
            chunk_create(chunk, ITEM_BRAIN_M);
            chunk_create(chunk, ITEM_DB_S);
            return star->coord;
        }
    }
}

struct chunk *world_chunk(struct world *world, struct coord coord)
{
    return sector_chunk(world_sector(world, coord), coord);
}

struct sector *world_sector(struct world *world, struct coord sector)
{
    struct coord coord = coord_sector(sector);
    uint64_t id = coord_to_id(coord);

    struct htable_ret ret = htable_get(&world->sectors, id);
    if (ret.ok) return (struct sector *) ret.value;

    struct sector *value = sector_gen(coord);
    ret = htable_put(&world->sectors, id, (uintptr_t) value);
    assert(ret.ok);

    return value;
}

const struct star *world_star(struct world *world, struct rect rect)
{
    // Very likely that the center of the rectangle is the right place to look
    // so make a first initial guess.
    struct sector *sector = world_sector(world, rect_center(&rect));
    const struct star *star = sector_star(sector, rect);
    if (star) return star;

    // Alright so our guess didn't work out so time for an exhaustive search.
    struct coord it = rect_next_sector(rect, coord_nil());
    for (; !coord_is_nil(it); it = rect_next_sector(rect, it)) {
        sector = world_sector(world, rect_center(&rect));
        star = sector_star(sector, rect);
        if (star) return star;
    }

    return NULL;
}

struct world_render_it world_render_it(struct world *world, struct rect viewport)
{
    return (struct world_render_it) {
        .rect = viewport,
        .sector = world_sector(world, viewport.top),
        .index = 0,
    };
}

const struct star *world_render_next(struct world *world, struct world_render_it *it)
{
    while (true) {
        if (it->index < it->sector->stars_len) {
            struct star *star = &it->sector->stars[it->index++];
            if (rect_contains(&it->rect, star->coord)) return star;
            continue;
        }

        struct coord coord = rect_next_sector(it->rect, it->sector->coord);
        if (coord_is_nil(coord)) return NULL;
        it->sector = world_sector(world, coord);
        it->index = 0;
    }
}


struct world_scan_it world_scan_it(struct world *world, struct coord coord)
{
    (void) world;
    return (struct world_scan_it) {
        .coord = coord_sector(coord),
        .index = 0,
    };
}

struct coord world_scan_next(struct world *world, struct world_scan_it *it)
{
    struct sector *sector = world_sector(world, it->coord);
    if (it->index >= sector->stars_len) return coord_nil();
    return sector->stars[it->index++].coord;
}


ssize_t world_scan(struct world *world, struct coord coord, item_t item)
{
    return sector_scan(world_sector(world, coord), coord, item);
}
