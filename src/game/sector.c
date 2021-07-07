/* galaxy.c
   Rémi Attab (remi.attab@gmail.com), 12 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/sector.h"
#include "game/chunk.h"
#include "utils/rng.h"

// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

void star_gen(struct star *star, struct coord coord)
{
    struct rng rng = rng_make(coord_to_id(coord));

    star->coord = coord;
    star->state = star_untouched;

    star->power = 1U << rng_uni(&rng, 1, 32);
    star->power += rng_uni(&rng, 1, star->power);

    size_t planets = rng_norm(&rng, 1, 16);
    for (size_t planet = 0; planet < planets; ++planet) {
        size_t size = rng_norm(&rng, 1, 16);
        for (size_t roll = 0; roll < size; ++roll) {
            size_t elem = rng_exp(&rng, 0, ITEMS_NATURAL_LEN);
            star->elems[elem] = u16_saturate_add(star->elems[elem], 1U << size);
        }
    }
}


bool star_load(struct star *star, struct save *save)
{
    if (!save_read_magic(save, save_magic_star)) return false;
    save_read_into(save, &star->coord);
    save_read_into(save, &star->state);
    save_read_into(save, &star->power);
    save_read_into(save, &star->elems);
    return save_read_magic(save, save_magic_star);
}

void star_save(struct star *star, struct save *save)
{
    save_write_magic(save, save_magic_star);
    save_write_value(save, star->coord);
    save_write_value(save, star->state);
    save_write_value(save, star->power);
    save_write_from(save, &star->elems);
    save_write_magic(save, save_magic_star);
}


// -----------------------------------------------------------------------------
// sector
// -----------------------------------------------------------------------------

static struct sector *sector_new(struct world *world, size_t stars)
{
    struct sector *sector =
        calloc(1, sizeof(*sector) + (stars * sizeof(sector->stars[0])));

    sector->world = world;
    sector->stars_len = stars;
    htable_reserve(&sector->index, stars);

    return sector;
}

struct sector *sector_gen(struct world *world, struct coord coord)
{
    coord = coord_sector(coord);
    struct rng rng = rng_make(coord_to_id(coord));

    double delta_max = coord_dist_2(coord_nil(), coord_center());
    double delta = coord_dist_2(coord, coord_center());

    enum { stars_max = 1U << 10 };
    size_t stars = (stars_max * (delta_max - delta)) / delta_max;

    size_t fuzz = rng_uni(&rng, 0, (stars / 4) * 2);
    stars = fuzz < stars ? stars - fuzz : stars + fuzz;

    struct sector *sector = sector_new(world, stars);
    sector->coord = coord;

    for (size_t i = 0; i < stars; ++i) {
        struct star *star = &sector->stars[i];

        struct coord pos = id_to_coord(rng_step(&rng));
        pos.x = coord.x | (pos.x & coord_sector_mask);
        pos.y = coord.y | (pos.y & coord_sector_mask);

        struct htable_ret ret =
            htable_put(&sector->index, coord_to_id(pos), (uintptr_t) star);
        if (!ret.ok) continue;

        star_gen(star, pos);
    }

    return sector;
}

void sector_free(struct sector *sector)
{
    free(sector);
}


struct sector *sector_load(struct world *world, struct save *save)
{
    if (!save_read_magic(save, save_magic_sector)) return NULL;

    struct coord coord = save_read_type(save, typeof(coord));
    struct sector *sector = sector_gen(world, coord);

    size_t chunks = save_read_type(save, uint32_t);
    htable_reserve(&sector->chunks, chunks);
    for (size_t i = 0; i < chunks; ++i) {
        struct chunk *chunk = chunk_load(world, save);
        if (!chunk) goto fail;

        uint64_t id = coord_to_id(chunk_star(chunk)->coord);

        struct htable_ret ret = htable_get(&sector->index, id);
        if (!ret.ok) goto fail;
        ((struct star *) ret.value)->state = star_active;

        ret = htable_put(&sector->chunks, id, (uintptr_t) chunk);
        assert(ret.ok);
    }

    if (!save_read_magic(save, save_magic_sector)) goto fail;
    return sector;

  fail:
    sector_free(sector);
    return NULL;
}

void sector_save(struct sector *sector, struct save *save)
{
    save_write_magic(save, save_magic_sector);
    save_write_value(save, sector->coord);
    save_write_value(save, (uint32_t) sector->chunks.len);
    struct htable_bucket *it = htable_next(&sector->chunks, NULL);
    for (; it; it = htable_next(&sector->chunks, it)) {
        chunk_save((struct chunk *) it->value, save);
    }

    save_write_magic(save, save_magic_sector);
}

struct chunk *sector_chunk_alloc(struct sector *sector, struct coord coord)
{
    uint64_t id = coord_to_id(coord);

    struct chunk *chunk = sector_chunk(sector, coord);
    if (likely(chunk != NULL)) return chunk;

    struct htable_ret ret = htable_get(&sector->index, id);
    if (!ret.ok) return NULL;
    struct star *star = (void *) ret.value;

    chunk = chunk_alloc(sector->world, star);
    ret = htable_put(&sector->chunks, id, (uint64_t) chunk);
    assert(ret.ok);

    star->state = star_active;
    return chunk;
}

struct chunk *sector_chunk(struct sector *sector, struct coord coord)
{
    uint64_t id = coord_to_id(coord);
    struct htable_ret ret = htable_get(&sector->chunks, id);
    return ret.ok ? (void *) ret.value : NULL;
}

const struct star *sector_star(struct sector *sector, struct rect rect)
{
    for (size_t i = 0; i < sector->stars_len; ++i) {
        struct star *star = &sector->stars[i];
        if (rect_contains(&rect, star->coord)) return star;
    }
    return NULL;
}

void sector_step(struct sector *sector)
{
    struct htable_bucket *bucket = htable_next(&sector->chunks, NULL);
    for (; bucket; bucket = htable_next(&sector->chunks, bucket))
        chunk_step((void *) bucket->value);
}

ssize_t sector_scan(struct sector *sector, struct coord coord, enum item item)
{
    uint64_t id = coord_to_id(coord);

    struct htable_ret ret = htable_get(&sector->index, id);
    if (!ret.ok) return -1;

    struct star *star = (void *) ret.value;
    if (star->state != star_active) {
        if (item < ITEM_NATURAL_FIRST && item >= ITEM_SYNTH_FIRST) return -1;
        return star->elems[item - ITEM_NATURAL_FIRST];
    }

    ret = htable_get(&sector->chunks, id);
    assert(ret.ok);
    return chunk_scan((struct chunk *) ret.value, item);
}


void sector_lanes_arrive(struct sector *sector,
        struct coord dst, enum item type, uint32_t data)
{
    struct chunk *chunk = sector_chunk_alloc(sector, dst);
    if (chunk) chunk_lanes_arrive(chunk, type, data);
}
