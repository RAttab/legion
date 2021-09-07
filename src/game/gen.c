/* gen.c
   Rémi Attab (remi.attab@gmail.com), 07 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/gen.h"
#include "game/sector.h"
#include "utils/rng.h"
#include "utils/vec.h"

// -----------------------------------------------------------------------------
// gen
// -----------------------------------------------------------------------------

enum
{
    gen_stars_min = 4,
    gen_stars_max = 1000,

    gen_square_size = 256,
    gen_square_margin = 128,
    gen_square_fuzz = gen_square_size - gen_square_margin,

    gen_square_num = 256,
    gen_square_total = gen_square_num * gen_square_num,
};

static_assert(gen_square_size > gen_square_margin);
static_assert(gen_square_num * gen_square_num > gen_stars_max * 2);
static_assert(gen_square_num * gen_square_size == coord_sector_size);
static_assert(gen_square_total > 0);


enum legion_packed roll_type
{
    roll_end = 0,
    roll_one,
    roll_rng,
    roll_n_of,
    roll_one_of,
    roll_all_of,
};

struct roll
{
    enum roll_type type;
    uint8_t rolls;
    enum item min, max;
    uint16_t count;
};

struct gen
{
    const char *name;
    uint64_t weight;
    struct roll rolls[8];
};


static struct rng gen_rng(struct coord coord, seed_t seed)
{
    return rng_make(coord_to_u64(coord) ^ seed);
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------


#define gen_end() { .type = roll_end }

#define gen_one(_item, _count) {                \
        .type = roll_one,                       \
        .min = _item,                           \
        .count = _count,                        \
    }

#define gen_rng(_min, _max, _count) {           \
        .type = roll_rng,                       \
        .min = _min,                            \
        .max = _max,                            \
        .count = _count,                        \
    }

#define gen_n_of(_rolls, _min, _max, _count) {  \
        .type = roll_n_of,                      \
        .rolls = _rolls,                        \
        .min = _min,                            \
        .max = _max,                            \
        .count = _count,                        \
    }

#define gen_one_of(_min, _max, _count) {        \
        .type = roll_one_of,                    \
        .min = _min,                            \
        .max = _max,                            \
        .count = _count,                        \
    }

#define gen_all_of(_min, _max, _count) {        \
        .type = roll_all_of,                    \
        .min = _min,                            \
        .max = _max,                            \
        .count = _count,                        \
    }

#define gen_star(_name, _weight, ...) {         \
        .name = #_name,                         \
        .weight = _weight,                      \
        .rolls = { __VA_ARGS__, gen_end() },    \
    }


static const struct gen gen_list[] = {
#include "gen_config.c"
};

#undef gen_end
#undef gen_one
#undef gen_rng
#undef gen_n_of
#undef gen_one_of
#undef gen_all_of
#undef gen_star


// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

static void gen_inc(struct star *star, enum item item, uint16_t inc)
{
    uint16_t *ptr = NULL;
    if (item == ITEM_ENERGY) ptr = &star->energy;
    else if (item >= ITEM_NATURAL_FIRST && item < ITEM_NATURAL_LAST)
        ptr = &star->elems[item - ITEM_NATURAL_FIRST];
    else assert(false);

    *ptr = u16_saturate_add(*ptr, inc);
}

static bool gen_roll(const struct roll *roll, struct star *star, struct rng *rng)
{
    const uint16_t max = roll->count;
    const uint16_t min = max / 10;

    switch (roll->type)
    {
    case roll_end: { return false; }

    case roll_one: {
        gen_inc(star, roll->min, rng_uni(rng, min, max));
        return true;
    }

    case roll_rng: {
        size_t n = rng_uni(rng, 0, roll->max - roll->min);
        for (size_t i = 0; i < n; ++i) {
            gen_inc(star,
                    rng_uni(rng, roll->min, roll->max + 1),
                    rng_uni(rng, min, max));
        }
        return true;
    }

    case roll_n_of: {
        for (size_t i = 0; i < roll->rolls; ++i) {
            gen_inc(star,
                    rng_uni(rng, roll->min, roll->max + 1),
                    rng_uni(rng, min, max));
        }
        return true;
    }

    case roll_one_of: {
        gen_inc(star,
                rng_uni(rng, roll->min, roll->max + 1),
                rng_uni(rng, min, max));
        return true;
    }

    case roll_all_of: {
        for (enum item it = roll->min; it <= roll->max; ++it)
            gen_inc(star, it, rng_uni(rng, min, max));
        return true;
    }

    default: { assert(false); }
    }
}

void gen_star(struct star *star, struct coord coord, seed_t seed)
{
    struct rng rng = gen_rng(coord, seed);
    star->coord = coord;

    static size_t weights = 0;
    if (unlikely(!weights)) {
        size_t sum = 0;
        for (size_t i = 0; i < array_len(gen_list); ++i)
            sum += gen_list[i].weight;
        weights = sum;
    }

    const struct gen *gen = gen_list;
    size_t pick = rng_uni(&rng, 0, weights);
    while (gen->weight < pick) { pick -= gen->weight; gen++; }

    const struct roll *roll = gen->rolls;
    while (gen_roll(roll, star, &rng)) roll++;
}


// -----------------------------------------------------------------------------
// sector
// -----------------------------------------------------------------------------

// Very elaborate way of avoiding star overlap without having to constantly look
// for collisions.
struct vec64 *gen_sector_stars(struct rng *rng, struct coord base, size_t stars)
{
    stars = legion_max(stars, (size_t) gen_stars_min);
    struct vec64 *list = vec64_reserve(stars * 2);

    uint64_t it = 0;
    const uint64_t step = gen_square_total / stars;

    while (true) {
        it += rng_uni(rng, 1, step * 2);
        if (it >= gen_square_total) break;

        struct coord coord = {
            .x = base.x + ((it % gen_square_num) * gen_square_size),
            .y = base.y + ((it / gen_square_num) * gen_square_size),
        };
        coord.x += gen_square_margin + rng_uni(rng, 0, gen_square_fuzz);
        coord.y += gen_square_margin + rng_uni(rng, 0, gen_square_fuzz);

        list = vec64_append(list, coord_to_u64(coord));
    }

    return list;
}

struct sector *gen_sector(struct world *world, struct coord coord, seed_t seed)
{
    coord = coord_sector(coord);
    struct rng rng = gen_rng(coord, seed);

    uint64_t mid = coord_mid;
    uint128_t delta_max = mid * mid;
    uint128_t delta = coord_dist_2(coord, coord_center());

    size_t stars = gen_stars_min;
    if (delta < delta_max)
        stars = (gen_stars_max * (delta_max - delta)) / delta_max;

    struct vec64 *list = gen_sector_stars(&rng, coord, stars);
    struct sector *sector = sector_new(world, vec64_len(list));
    sector->coord = coord;

    for (size_t i = 0; i < sector->stars_len; ++i) {
        struct star *star = &sector->stars[i];

        struct coord coord_star = coord_from_u64(list->vals[i]);
        gen_star(star, coord_star, seed);

        struct htable_ret ret =
            htable_put(&sector->index, coord_to_u64(coord_star), (uintptr_t) star);
        if (!ret.ok) continue;
    }

    vec64_free(list);
    return sector;
}
