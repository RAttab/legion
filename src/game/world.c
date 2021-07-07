/* world.c
   Rémi Attab (remi.attab@gmail.com), 30 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/world.h"
#include "vm/mod.h"


// -----------------------------------------------------------------------------
// world
// -----------------------------------------------------------------------------


struct world
{
    world_ts_t time;
    struct mods *mods;
    struct htable sectors;
    struct lanes lanes;
};

struct world *world_new(void)
{
    struct world *world = calloc(1, sizeof(*world));
    world->mods = mods_new();
    lanes_init(&world->lanes, world);
    htable_reset(&world->sectors);
    return world;
}

void world_free(struct world *world)
{
    struct htable_bucket *it = htable_next(&world->sectors, NULL);
    for (; it; it = htable_next(&world->sectors, it))
        sector_free((struct sector *) it->value);
    htable_reset(&world->sectors);
    lanes_free(&world->lanes);
    mods_free(world->mods);
    free(world);
}

struct world *world_load(struct save *save)
{
    if (!save_read_magic(save, save_magic_world)) return NULL;

    struct world *world = world_new();
    save_read_into(save, &world->time);

    if (!(world->mods = mods_load(save))) goto fail;
    if (!lanes_load(&world->lanes, world, save)) goto fail;

    uint32_t sectors = save_read_type(save, uint32_t);
    htable_reserve(&world->sectors, sectors);

    for (size_t i = 0; i < sectors; ++i) {
        struct sector *sector = sector_load(world, save);
        if (!sector) goto fail;

        uint64_t id = coord_to_id(sector->coord);
        struct htable_ret ret = htable_put(&world->sectors, id, (uintptr_t) sector);
        assert(ret.ok);
    }

    if (!save_read_magic(save, save_magic_world)) goto fail;
    return world;

  fail:
    world_free(world);
    return NULL;
}

void world_save(struct world *world, struct save *save)
{
    save_write_magic(save, save_magic_world);
    save_write_value(save, world->time);
    mods_save(world->mods, save);
    lanes_save(&world->lanes, save);

    uint32_t sectors = 0;
    struct htable_bucket *it = htable_next(&world->sectors, NULL);
    for (; it; it = htable_next(&world->sectors, it)) {
        struct sector *sector = (void *) it->value;
        if (sector->chunks.len) sectors++;
    }
    save_write_value(save, sectors);

    it = htable_next(&world->sectors, NULL);
    for (; it; it = htable_next(&world->sectors, it)) {
        struct sector *sector = (void *) it->value;
        if (sector->chunks.len) sector_save(sector, save);
    }

    save_write_magic(save, save_magic_world);
}

void world_step(struct world *world)
{
    world->time++;
    lanes_step(&world->lanes);

    struct htable_bucket *it = htable_next(&world->sectors, NULL);
    for (; it; it = htable_next(&world->sectors, it))
        sector_step((struct sector *) it->value);
}

struct coord world_populate(struct world *world)
{
    mods_populate(world->mods);

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
            assert(chunk);

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

world_ts_t world_time(struct world *world)
{
    return world->time;
}

struct mods *world_mods(struct world *world)
{
    return world->mods;
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

    struct sector *value = sector_gen(world, coord);
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


ssize_t world_scan(struct world *world, struct coord coord, enum item item)
{
    return sector_scan(world_sector(world, coord), coord, item);
}


void world_lanes_launch(struct world *world,
        struct coord src, struct coord dst, enum item type, uint32_t data)
{
    lanes_launch(&world->lanes, src, dst, type, cargo, count);
}

void world_lanes_arrive(struct world *world,
        struct coord dst, enum item type, uint32_t data)
{
    sector_lanes_arrive(world_sector(world, dst), dst, type, data);
}
