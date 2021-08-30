/* world.c
   Rémi Attab (remi.attab@gmail.com), 30 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/world.h"
#include "vm/mod.h"
#include "vm/atoms.h"
#include "game/tape.h"
#include "items/config.h"
#include "items/legion/legion.h"


// -----------------------------------------------------------------------------
// world
// -----------------------------------------------------------------------------


struct world
{
    world_ts_t time;

    struct tape_set known;
    struct tape_set learned;
    struct htable research;

    struct mods *mods;
    struct atoms *atoms;

    struct htable sectors;
    struct lanes lanes;
};

struct world *world_new(void)
{
    struct world *world = calloc(1, sizeof(*world));

    world->atoms = atoms_new();
    world->mods = mods_new();
    lanes_init(&world->lanes, world);
    htable_reset(&world->sectors);
    htable_reset(&world->research);

    return world;
}

void world_free(struct world *world)
{
    atoms_free(world->atoms);
    mods_free(world->mods);
    lanes_free(&world->lanes);

    for (struct htable_bucket *it = htable_next(&world->sectors, NULL);
         it; it = htable_next(&world->sectors, it))
        sector_free((struct sector *) it->value);
    htable_reset(&world->sectors);

    htable_reset(&world->research);
    free(world);
}

struct world *world_load(struct save *save)
{
    if (!save_read_magic(save, save_magic_world)) return NULL;

    struct world *world = world_new();
    save_read_into(save, &world->time);

    if (!tape_set_load(&world->known, save)) goto fail;
    if (!tape_set_load(&world->learned, save)) goto fail;
    if (!save_read_htable(save, &world->research)) goto fail;

    atoms_free(world->atoms);
    if (!(world->atoms = atoms_load(save))) goto fail;

    mods_free(world->mods);
    if (!(world->mods = mods_load(save))) goto fail;

    if (!lanes_load(&world->lanes, world, save)) goto fail;

    uint32_t sectors = save_read_type(save, uint32_t);
    htable_reserve(&world->sectors, sectors);

    for (size_t i = 0; i < sectors; ++i) {
        struct sector *sector = sector_load(world, save);
        if (!sector) goto fail;

        uint64_t id = coord_to_u64(sector->coord);
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

    tape_set_save(&world->known, save);
    tape_set_save(&world->learned, save);
    save_write_htable(save, &world->research);

    atoms_save(world->atoms, save);
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

world_ts_t world_time(struct world *world)
{
    return world->time;
}

struct atoms *world_atoms(struct world *world)
{
    return world->atoms;
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
    uint64_t id = coord_to_u64(coord);

    struct htable_ret ret = htable_get(&world->sectors, id);
    if (ret.ok) return (struct sector *) ret.value;

    struct sector *value = sector_gen(world, coord);
    ret = htable_put(&world->sectors, id, (uintptr_t) value);
    assert(ret.ok);

    return value;
}

const struct star *world_star_in(struct world *world, struct rect rect)
{
    // Very likely that the center of the rectangle is the right place to look
    // so make a first initial guess.
    struct sector *sector = world_sector(world, rect_center(&rect));
    const struct star *star = sector_star_in(sector, rect);
    if (star) return star;

    // Alright so our guess didn't work out so time for an exhaustive search.
    struct coord it = rect_next_sector(rect, coord_nil());
    for (; !coord_is_nil(it); it = rect_next_sector(rect, it)) {
        sector = world_sector(world, rect_center(&rect));
        star = sector_star_in(sector, rect);
        if (star) return star;
    }

    return NULL;
}

const struct star *world_star_at(struct world *world, struct coord coord)
{
    return sector_star_at(world_sector(world, coord), coord);
}


// -----------------------------------------------------------------------------
// render
// -----------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------
// scan
// -----------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

const struct hset *world_lanes_list(struct world *world, struct coord key)
{
    return lanes_list(&world->lanes, key);
}

void world_lanes_launch(
        struct world *world,
        enum item type, size_t speed,
        struct coord src, struct coord dst,
        const word_t *data, size_t len)
{
    lanes_launch(&world->lanes, type, speed, src, dst, data, len);
}

void world_lanes_arrive(
        struct world *world,
        enum item type,
        struct coord src, struct coord dst,
        const word_t *data, size_t len)
{
    sector_lanes_arrive(world_sector(world, dst), type, src, dst, data, len);
}


// -----------------------------------------------------------------------------
// lab
// -----------------------------------------------------------------------------

bool world_lab_known(struct world *world, enum item item)
{
    assert(item);
    return tape_set_check(&world->known, item);
}

struct tape_set world_lab_known_list(struct world *world)
{
    return world->known;
}

void world_lab_learn(struct world *world, enum item item)
{
    assert(item);
    tape_set_put(&world->learned, item);

    struct tape_set todo = tape_set_invert(&world->known);
    for (enum item it = tape_set_next(&todo, 0); it; it = tape_set_next(&todo, it)) {
        const struct tape_info *info = tapes_info(it);
        if (!info) continue;

        size_t intersect = tape_set_intersect(&world->learned, &info->reqs);
        if (intersect == tape_set_len(&info->reqs))
            tape_set_put(&world->known, it);
    }
}

bool world_lab_learned(struct world *world, enum item item)
{
    assert(item);
    return tape_set_check(&world->learned, item);
}

struct tape_set world_lab_learned_list(struct world *world)
{
    return world->learned;
}

uint64_t world_lab_learned_bits(struct world *world, enum item item)
{
    assert(item);

    const uint8_t bits = im_config_assert(item)->lab_bits;
    const uint64_t mask = (1ULL << bits) - 1;
    if (tape_set_check(&world->learned, item)) return mask;

    struct htable_ret ret = htable_get(&world->research, item);
    return ret.ok ? ret.value : 0;
}

void world_lab_learn_bit(struct world *world, enum item item, uint8_t bit)
{
    const uint8_t bits = im_config_assert(item)->lab_bits;
    assert(bit < bits);

    if (tape_set_check(&world->learned, item)) return;

    struct htable_ret ret = htable_get(&world->research, item);
    uint64_t value = ret.ok ? ret.value : 0;
    value |= 1ULL << bit;

    const uint64_t mask = (1ULL << bits) - 1;
    if (value != mask) {
        if (ret.ok) ret = htable_xchg(&world->research, item, value);
        else ret = htable_put(&world->research, item, value);
        assert(ret.ok);
        return;
    }

    world_lab_learn(world, item);
    ret = htable_del(&world->research, item);
    assert(ret.ok);
}


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

struct coord world_populate(struct world *world)
{
    im_populate_atoms(world->atoms);
    mods_populate(world->mods, world->atoms);

    tape_set_put(&world->known, ITEM_NIL);
    for (enum item it = ITEM_NATURAL_FIRST; it < ITEM_NATURAL_LAST; ++it)
        tape_set_put(&world->known, it);

    tape_set_put(&world->known, ITEM_LEGION);
    tape_set_union(&world->known, &tapes_info(ITEM_LEGION)->reqs);

    tape_set_put(&world->known, ITEM_LAB);
    tape_set_union(&world->known, &tapes_info(ITEM_LAB)->reqs);

    struct rng rng = rng_make(0);
    while (true) {
        struct coord coord = coord_from_u64(rng_step(&rng));
        struct sector *sector = world_sector(world, coord);
        if (sector->stars_len < 100) continue;

        struct star *star = NULL;
        for (size_t tries = 0; tries < 10; ++tries) {
            size_t index = rng_uni(&rng, 0, sector->stars_len);
            star = &sector->stars[index];

            if (star->energy < 10000) continue;
            if (star_elem(star, ITEM_ELEM_A) < 20000) continue;
            if (star_elem(star, ITEM_ELEM_B) < 20000) continue;
            if (star_elem(star, ITEM_ELEM_C) < 20000) continue;
            if (star_elem(star, ITEM_ELEM_D) < 20000) continue;
            if (star_elem(star, ITEM_ELEM_E) < 20000) continue;
            if (star_elem(star, ITEM_ELEM_F) < 20000) continue;
            if (star_elem(star, ITEM_ELEM_G) < 2000) continue;
            if (star_elem(star, ITEM_ELEM_H) < 2000) continue;

            struct chunk *chunk = sector_chunk_alloc(sector, star->coord);
            assert(chunk);

            for (const enum item *it = im_legion_cargo(ITEM_LEGION); *it; it++)
                chunk_create(chunk, *it);
            chunk_create(chunk, ITEM_LEGION);

            return star->coord;
        }
    }
}
