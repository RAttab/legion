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


// -----------------------------------------------------------------------------
// sector
// -----------------------------------------------------------------------------

struct sector *sector_gen(struct coord coord)
{
    coord = coord_sector(coord);
    struct rng rng = rng_make(coord_to_id(coord));

    size_t stars = rng_uni(&rng, 1, 1U << 10);

    struct sector *sector =
        calloc(1, sizeof(*sector) + (stars * sizeof(sector->stars[0])));
    sector->coord = coord;
    sector->stars_len = stars;

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


struct chunk *sector_chunk_alloc(struct sector *sector, struct coord coord)
{
    uint64_t id = coord_to_id(coord);

    struct chunk *chunk = sector_chunk(sector, coord);
    if (likely(chunk != NULL)) return chunk;

    struct htable_ret ret = htable_get(&sector->index, id);
    struct star *star = (void *) ret.value;
    assert(ret.ok);

    chunk = chunk_alloc(star);
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

ssize_t sector_scan(struct sector *sector, struct coord coord, item_t item)
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
