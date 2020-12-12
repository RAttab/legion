/* galaxy.c
   Rémi Attab (remi.attab@gmail.com), 12 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#include "galaxy.h"
#include "game/hunk.h"
#include "utils/rng.h"

// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

void star_gen(struct star *star, struct coord coord)
{
    uint64_t id = coord_to_id(coord);
    struct rng rng = rng_make(id);

    star->coord = coord;
    star->state = star_untouched;

    star->power = 1U << rng_uni(&rng, 1, 32);
    star->power += rng_uni(&rng, 1, star->power);

    size_t planets = rng_norm(&rng, 1, 16);
    for (size_t planet = 0; planet < planets; ++planet) {
        size_t size = rng_uni(&rng, 1, 16);
        for (size_t roll = 0; roll < size; ++roll) {
            size_t ele = rng_exp(&rng, ele_natural_min, ele_natural_max + 1);
            star->elements[ele] = u16_saturate_add(star_elements[ele], 1U << size);
        }
    }
}


// -----------------------------------------------------------------------------
// sector
// -----------------------------------------------------------------------------

struct sector *sector_gen(struct coord coord)
{
    coord = coord_sector(coord);

    uint64_t id = coord_to_id(coord);
    struct rng rng = rng_make(id);

    size_t stars = rng_uni(&rng, 1, 1U << 10);

    struct sector *sector =
        calloc(1, sizeof(*sector) + (stars * sizeof(sector->stars[0])));
    sector->coord = coord;
    sector->stars_len = stars;

    for (size_t i = 0; i < stars; ++i) {
        uint64_t value = rng_step(&rng);

        struct coord star = {
            .x = value & coord_sector_mask,
            .y = (value >> coord_sector_bits) & coord_sector_mask,
        };
        star_gen(&sector->stars[i], star,
    }
}


struct hunk *sector_hunk(struct sector *sector, struct coord coord)
{
    uint64_t id = coord_to_id(coord);

    struct hunk *hunk = sector_hunk_get(&sector->hunks, id);
    if (likely(hunk)) return hunk;

    hunk = hunk_alloc(coord);
    struct htable_ret ret = htable_put(&sector->hunks, id, (uint64_t) hunk);
    assert(ret.ok);

    return hunk;
}

struct hunk *sector_hunk_get(struct sector *sector, struct coord coord)
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
