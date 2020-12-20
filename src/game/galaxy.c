/* galaxy.c
   Rémi Attab (remi.attab@gmail.com), 12 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "galaxy.h"
#include "game/hunk.h"
#include "game/obj.h"
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
            size_t elem = rng_exp(&rng, 0, elem_natural_len);
            star->elements[elem] =
                u16_saturate_add(star->elements[elem], 1U << size);
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
        uint64_t value = rng_step(&rng);

        struct coord star_coord = {
            .x = value & coord_sector_mask,
            .y = (value >> coord_sector_bits) & coord_sector_mask,
        };

        // \todo index with qtree to check for proximity.
        struct htable_ret ret =
            htable_put(&sector->index, coord_to_id(star_coord), (uint64_t) star);
        if (!ret.ok) continue;

        star_gen(star, star_coord);
    }

    return sector;
}


struct hunk *sector_hunk_alloc(struct sector *sector, struct coord coord)
{
    uint64_t id = coord_to_id(coord);

    struct hunk *hunk = sector_hunk(sector, coord);
    if (likely(hunk != NULL)) return hunk;

    struct htable_ret ret = htable_get(&sector->index, id);
    struct star *star = (void *) ret.value;
    assert(ret.ok);

    hunk = hunk_alloc(star);
    ret = htable_put(&sector->hunks, id, (uint64_t) hunk);
    assert(ret.ok);

    star->state = star_active;
    return hunk;
}

struct hunk *sector_hunk(struct sector *sector, struct coord coord)
{
    uint64_t id = coord_to_id(coord);
    struct htable_ret ret = htable_get(&sector->hunks, id);
    return ret.ok ? (void *) ret.value : NULL;
}

void sector_step(struct sector *sector)
{
    struct htable_bucket *bucket = htable_next(&sector->hunks, NULL);
    for (; bucket; bucket = htable_next(&sector->hunks, bucket))
        hunk_step((void *) bucket->value);
}

const struct star *sector_star(struct sector *sector, const struct rect *rect)
{
    for (size_t i = 0; i < sector->stars_len; ++i) {
        struct star *star = &sector->stars[i];
        if (rect_contains(rect, star->coord)) return star;
    }
    return NULL;
}

void sector_preload(struct sector *sector)
{
    struct rng rng = rng_make(coord_to_id(sector->coord));
    size_t i = rng_uni(&rng, 0, sector->stars_len);
    struct star *star = &sector->stars[i];

    struct hunk *hunk = sector_hunk_alloc(sector, star->coord);
    for (size_t i = 0; i < 5; ++i) worker_alloc(hunk);
}
