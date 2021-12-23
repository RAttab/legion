/* gen.c
   Rémi Attab (remi.attab@gmail.com), 07 Sep 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/sys.h"
#include "game/gen.h"
#include "game/world.h"
#include "game/sector.h"
#include "vm/token.h"
#include "vm/symbol.h"
#include "vm/atoms.h"
#include "utils/fs.h"
#include "utils/rng.h"
#include "utils/vec.h"
#include "utils/hash.h"


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

static struct
{
    struct gen stars[8];
    size_t stars_range;

    size_t prefix_len;
    const struct symbol *prefix;

    size_t suffix_len;
    const struct symbol *suffix;
} gen;


static uint64_t gen_mix(struct coord coord, seed_t seed)
{
    return coord_to_u64(coord) ^ seed;
}

static struct rng gen_rng(struct coord coord, seed_t seed)
{
    return rng_make(gen_mix(coord, seed));
}

// -----------------------------------------------------------------------------
// name
// -----------------------------------------------------------------------------

word_t gen_name(struct coord coord, seed_t seed, struct atoms *atoms)
{
    uint64_t bits = hash_u64(gen_mix(coord, seed));

    const struct symbol *prefix = &gen.prefix[u64_top(bits) % gen.prefix_len];
    const struct symbol *suffix = &gen.suffix[u64_bot(bits) % gen.suffix_len];
    assert(prefix->len + suffix->len + 1 < symbol_cap);

    struct symbol name = *prefix;
    name.c[name.len] = '-';
    name.len++;
    memcpy(name.c + name.len, suffix->c, suffix->len);
    name.len += suffix->len;

    return atoms_make(atoms, &name);
}


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

struct sector *gen_sector(struct coord coord, seed_t seed)
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


static enum roll_type gen_populate_roll_type(struct tokenizer *tok)
{
    struct token token = {0};
    assert(token_expect(tok, &token, token_symbol));
    uint64_t hash = symbol_hash(&token.value.s);

    if (hash == symbol_hash_c("one")) return roll_one;
    else if (hash == symbol_hash_c("rng")) return roll_rng;
    else if (hash == symbol_hash_c("one_of")) return roll_one_of;
    else if (hash == symbol_hash_c("all_of")) return roll_all_of;

    token_err(tok, "invalid roll type: %s", token.value.s.c);
    return roll_end;
}

static enum item gen_populate_roll_item(struct tokenizer *tok, struct atoms *atoms)
{
    struct token token = {0};
    assert(token_expect(tok, &token, token_symbol));

    word_t item = atoms_get(atoms, &token.value.s);
    if (item == ITEM_ENERGY) return item;
    if (item >= ITEM_NATURAL_FIRST && item < ITEM_NATURAL_LAST) return item;

    token_err(tok, "invalid item atom: %s", token.value.s.c);
    return ITEM_ELEM_A;
}

static uint16_t gen_populate_roll_count(struct tokenizer *tok)
{
    struct token token = {0};
    assert(token_expect(tok, &token, token_number));

    word_t count = token.value.w;
    if (count < 0 || count > UINT16_MAX) {
        token_err(tok, "invalid count: %lx", token.value.w);
        return 0;
    }

    return count;
}

static void gen_populate_rolls(
        struct tokenizer *tok, struct gen *gen, struct atoms *atoms)
{
    struct roll *it = gen->rolls;

    struct token token = {0};
    while (token_next(tok, &token)->type != token_close) {
        assert(token_assert(tok, &token, token_open));

        assert((size_t) (it - gen->rolls) < array_len(gen->rolls));

        it->type = gen_populate_roll_type(tok);

        switch (it->type)
        {
        case roll_one: {
            it->min = gen_populate_roll_item(tok, atoms);
            it->count = gen_populate_roll_count(tok);
            break;
        }

        case roll_rng:
        case roll_one_of:
        case roll_all_of: {
            it->min = gen_populate_roll_item(tok, atoms);
            it->max = gen_populate_roll_item(tok, atoms);
            it->count = gen_populate_roll_count(tok);
            break;
        }

        default: { assert(false); }
        }

        assert(token_expect(tok, &token, token_close));
        it++;
    }

    assert((size_t) (it - gen->rolls) < array_len(gen->rolls));
    it->type = roll_end;
}

static void gen_populate_stars(struct atoms *atoms)
{
    char path[PATH_MAX] = {0};
    sys_path_res("gen/stars.lisp", path, sizeof(path));
    struct mfile file = mfile_open(path);

    struct tokenizer tok = {0};
    struct token_ctx *ctx = token_init_stderr(&tok, path, file.ptr, file.len);

    struct gen *it = gen.stars;

    struct token token = {0};
    while (token_next(&tok, &token)->type != token_nil) {
        assert(token_assert(&tok, &token, token_open));

        assert((size_t) (it - gen.stars) < array_len(gen.stars));

        assert(token_expect(&tok, &token, token_symbol));
        it->name = token.value.s;

        while (token_next(&tok, &token)->type != token_close) {
            assert(token_assert(&tok, &token, token_open));

            assert(token_expect(&tok, &token, token_symbol));
            uint64_t hash = symbol_hash(&token.value.s);

            if (hash == symbol_hash_c("rolls"))
                gen_populate_rolls(&tok, it, atoms);

            else if (hash == symbol_hash_c("hue")) {
                assert(token_expect(&tok, &token, token_number));
                assert(token.value.w >= 0 && token.value.w < 360);
                it->hue = token.value.w;
                assert(token_expect(&tok, &token, token_close));
            }

            else if (hash == symbol_hash_c("weight")) {
                assert(token_expect(&tok, &token, token_number));
                it->weight = token.value.w;
                gen.stars_range += token.value.w;
                assert(token_expect(&tok, &token, token_close));
            }

            else assert(false);
        }

        it++;
    }

    assert(token_ctx_ok(ctx));
    token_ctx_free(ctx);
    mfile_close(&file);
}


static struct symbol *gen_populate_affix(const char *rel, size_t limit, size_t *ret)
{
    char path[PATH_MAX] = {0};
    sys_path_res(rel, path, sizeof(path));
    struct mfile file = mfile_open(path);

    struct tokenizer tok = {0};
    struct token_ctx *ctx = token_init_stderr(&tok, path, file.ptr, file.len);

    size_t len = 0, cap = 32;
    struct symbol *list = calloc(1, cap * sizeof(*list));

    struct token token = {0};
    while (token_next(&tok, &token)->type != token_nil) {
        assert(token_assert(&tok, &token, token_symbol));

        if (token.value.s.len > limit) {
            token_err(&tok, "affix too long: %s", token.value.s.c);
            continue;
        }

        if (len == cap) list = realloc(list, (cap *= 2) * sizeof(*list));
        list[len] = token.value.s;
        len++;
    }

    assert(token_ctx_ok(ctx));
    token_ctx_free(ctx);
    mfile_close(&file);

    *ret = len;
    return realloc(list, len * sizeof(*list));
}


void gen_populate(struct atoms *atoms)
{
    gen_populate_stars(atoms);
    gen.prefix = gen_populate_affix("gen/prefix.lisp", 19, &gen.prefix_len);
    gen.suffix = gen_populate_affix("gen/suffix.lisp", 10, &gen.suffix_len);
}
