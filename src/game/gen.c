/* gen.c
   Rémi Attab (remi.attab@gmail.com), 07 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/sys.h"
#include "game/gen.h"
#include "game/world.h"
#include "game/sector.h"
#include "vm/atoms.h"
#include "utils/symbol.h"
#include "utils/fs.h"
#include "utils/rng.h"
#include "utils/vec.h"
#include "utils/hash.h"
#include "utils/config.h"


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

    gen_prefix_cap = 15,
    gen_suffix_cap = 14,
};

static_assert(gen_square_size > gen_square_margin);
static_assert(gen_square_num * gen_square_num > gen_stars_max * 2);
static_assert(gen_square_num * gen_square_size == coord_sector_size);
static_assert(gen_square_total > 0);
static_assert(gen_prefix_cap + gen_suffix_cap + 1 == symbol_cap);


enum roll_type : uint8_t
{
    roll_end = 0,
    roll_one,
    roll_rng,
    roll_one_of,
    roll_all_of,
};

struct roll
{
    enum roll_type type;
    enum item min, max;
    uint16_t count;
};

struct gen
{
    struct symbol name;
    uint64_t weight;
    uint16_t hue;
    struct roll rolls[8];
};

struct affix
{
    size_t len;
    struct symbol name;
    struct symbol *list;
};

static struct
{
    struct gen stars[8];
    size_t stars_range;

    struct affix prefix;
    struct { size_t len; struct affix *list; } suffix;
} gen;


static uint64_t gen_mix(struct coord coord, world_seed seed)
{
    return coord_to_u64(coord) ^ seed;
}

static struct rng gen_rng(struct coord coord, world_seed seed)
{
    return rng_make(gen_mix(coord, seed));
}

// -----------------------------------------------------------------------------
// name
// -----------------------------------------------------------------------------

struct gen_bits { size_t prefix, suffix; };

struct gen_bits gen_name_bits(struct coord coord, world_seed seed, size_t suffixes)
{
    uint64_t bits = hash_u64(gen_mix(coord, seed));
    return (struct gen_bits) {
        .prefix = u64_top(bits) % gen.prefix.len,
        .suffix = u64_bot(bits) % suffixes,
    };
}

static struct symbol gen_name_symbol(
        const struct symbol *prefix,
        const struct symbol *suffix)
{
    assert(prefix->len + suffix->len + 1 < symbol_cap);

    struct symbol name = *prefix;
    name.c[name.len] = '-';
    name.len++;
    memcpy(name.c + name.len, suffix->c, suffix->len);
    name.len += suffix->len;
    return name;

}

struct symbol gen_name_sector(struct coord coord, world_seed seed)
{
    struct gen_bits bits = {0};

    bits = gen_name_bits(coord_sector(coord), seed, gen.suffix.len);
    struct affix *suffix = gen.suffix.list + bits.suffix;

    return gen_name_symbol(gen.prefix.list + bits.prefix, &suffix->name);
}

vm_word gen_name_star(struct coord coord, world_seed seed, struct atoms *atoms)
{
    struct gen_bits bits = {0};

    bits = gen_name_bits(coord_sector(coord), seed, gen.suffix.len);
    struct affix *suffix = gen.suffix.list + bits.suffix;

    bits = gen_name_bits(coord, seed, suffix->len);
    struct symbol name = gen_name_symbol(
            gen.prefix.list + bits.prefix, suffix->list + bits.suffix);

    return atoms_make(atoms, &name);
}


// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

static void gen_inc(struct star *star, enum item item, uint16_t inc)
{
    uint16_t *ptr = NULL;
    if (item == item_energy) ptr = &star->energy;
    else if (item_is_natural(item))
        ptr = &star->elems[item - items_natural_first];
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

void gen_star(struct star *star, struct coord coord, world_seed seed)
{
    struct rng rng = gen_rng(coord, seed);
    star->coord = coord;

    uint64_t pick = rng_uni(&rng, 0, gen.stars_range);
    assert(pick < gen.stars_range);

    const struct gen *it = gen.stars;
    while (it->weight < pick) { pick -= it->weight; it++; }

    const struct roll *roll = it->rolls;
    while (gen_roll(roll, star, &rng)) roll++;

    int16_t hue = it->hue + (15 - rng_uni(&rng, 0, 30));
    star->hue = hue < 0 ? 360 + hue : hue;
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

struct sector *gen_sector(struct coord coord, world_seed seed)
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
    struct sector *sector = sector_new(vec64_len(list));
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


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------


static enum roll_type gen_populate_roll_type(struct reader *in)
{
    struct symbol type = reader_symbol(in);
    uint64_t hash = symbol_hash(&type);

    if (hash == symbol_hash_c("one")) return roll_one;
    else if (hash == symbol_hash_c("rng")) return roll_rng;
    else if (hash == symbol_hash_c("one-of")) return roll_one_of;
    else if (hash == symbol_hash_c("all-of")) return roll_all_of;

    reader_err(in, "invalid roll type: %s", type.c);
    return roll_end;
}

static enum item gen_populate_roll_item(struct reader *in, struct atoms *atoms)
{
    vm_word item = reader_atom(in, atoms);
    if (item == item_energy) return item;
    if (item_is_natural(item)) return item;

    reader_err(in, "invalid item atom: %lx", item);
    return item_elem_a;
}

static uint16_t gen_populate_roll_count(struct reader *in)
{
    vm_word count = reader_word(in);
    if (count >= 0 && count <= UINT16_MAX) return count;

    reader_err(in, "invalid count: %lx", count);
    return 0;
}

static void gen_populate_rolls(
        struct reader *in, struct gen *gen, struct atoms *atoms)
{
    struct roll *it = gen->rolls;

    while (!reader_peek_close(in)) {
        reader_open(in);
        assert((size_t) (it - gen->rolls) < array_len(gen->rolls));

        it->type = gen_populate_roll_type(in);

        switch (it->type)
        {
        case roll_one: {
            it->min = gen_populate_roll_item(in, atoms);
            it->count = gen_populate_roll_count(in);
            break;
        }

        case roll_rng:
        case roll_one_of:
        case roll_all_of: {
            it->min = gen_populate_roll_item(in, atoms);
            it->max = gen_populate_roll_item(in, atoms);
            it->count = gen_populate_roll_count(in);
            break;
        }

        default: { assert(false); }
        }

        reader_close(in);
        it++;
    }

    assert((size_t) (it - gen->rolls) < array_len(gen->rolls));
    it->type = roll_end;
}

static void gen_populate_stars(struct atoms *atoms)
{
    char path[PATH_MAX] = {0};
    sys_path_res("gen/stars.lisp", path, sizeof(path));

    struct config config = {0};
    struct reader *in = config_read(&config, path);

    struct gen *it = gen.stars;

    while (!reader_peek_eof(in)) {
        reader_open(in);
        assert((size_t) (it - gen.stars) < array_len(gen.stars));

        it->name = reader_symbol(in);

        vm_word hue = reader_field(in, "hue", word);
        assert(hue >= 0 && hue < 360);
        it->hue = hue;

        it->weight = reader_field(in, "weight", word);
        gen.stars_range += it->weight;

        reader_open(in);
        reader_symbol_str(in, "rolls");
        gen_populate_rolls(in, it, atoms);
        reader_close(in);

        reader_close(in);
        it++;
    }

    config_close(&config);
}

static void gen_populate_affix(
        struct affix *affix, struct reader *in, size_t limit)
{
    size_t cap = 8;
    affix->list = calloc(cap, sizeof(*affix->list));

    reader_open(in);
    affix->name = reader_symbol(in);
    if (affix->name.len > gen_suffix_cap)
        reader_err(in, "affix too long: '%s' > %zu", affix->name.c, limit);

    while (!reader_peek_close(in)) {
        if (affix->len == cap) {
            size_t new = cap * 2;
            affix->list = realloc_zero(affix->list, cap, new, sizeof(*affix->list));
            cap = new;
        }

        struct symbol *it = affix->list + affix->len;
        *it = reader_symbol(in);
        if (it->len > limit)
            reader_err(in, "affix too long: '%s' > %zu", it->c, limit);

        affix->len++;
    }

    reader_close(in);
}

static void gen_populate_prefix(void)
{
    char path[PATH_MAX] = {0};
    sys_path_res("gen/prefix.lisp", path, sizeof(path));

    struct config config = {0};
    struct reader *in = config_read(&config, path);

    gen_populate_affix(&gen.prefix, in, gen_prefix_cap);

    config_close(&config);
}

static void gen_populate_suffix(void)
{
    char path[PATH_MAX] = {0};
    sys_path_res("gen/suffix.lisp", path, sizeof(path));

    struct config config = {0};
    struct reader *in = config_read(&config, path);

    size_t cap = 1;
    gen.suffix.list = calloc(cap, sizeof(*gen.suffix.list));

    while (!reader_peek_eof(in)) {
        if (gen.suffix.len == cap) {
            size_t new = cap * 2;
            gen.suffix.list = realloc_zero(gen.suffix.list, cap, new, sizeof(*gen.suffix.list));
            cap = new;
        }

        gen_populate_affix(gen.suffix.list + gen.suffix.len, in, gen_suffix_cap);
        gen.suffix.len++;
    }

    config_close(&config);
}


void gen_populate(struct atoms *atoms)
{
    gen_populate_stars(atoms);
    gen_populate_prefix();
    gen_populate_suffix();
}
