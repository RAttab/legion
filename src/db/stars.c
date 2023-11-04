/* stars.c
   Rémi Attab (remi.attab@gmail.com), 19 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// names
// -----------------------------------------------------------------------------

static struct
{
    struct stars_names *prefix;
    struct { size_t len; struct stars_names *list[16]; } suffix;
} stars_names;

const struct stars_names *stars_names_prefixes(void)
{
    return stars_names.prefix;
}

const struct stars_names *stars_names_suffixes(uint64_t rng)
{
    return stars_names.suffix.list[rng % stars_names.suffix.len];
}

const struct symbol *stars_names_pick(
        const struct stars_names *names, uint64_t rng)
{
    return names->list + (rng % names->len);
}

static void stars_names_populate(void)
{
    {
#define stars_prefix_begin(_len)                                \
    stars_names.prefix = calloc(1,                              \
            sizeof(*stars_names.prefix) +                       \
            sizeof(stars_names.prefix->list[0]) * _len);        \
    stars_names.prefix->len = _len;                             \
    stars_names.prefix->name = make_symbol("prefix");           \

#define stars_prefix(_ix, _name)                \
    stars_names.prefix->list[_ix] =             \
        make_symbol_len(_name, sizeof(_name));

#define stars_prefix_end()

#include "gen/stars_prefix.h"

#undef stars_prefix_begin
#undef stars_prefix
#undef stars_prefix_end
    }

    {
        size_t it = 0;

#define stars_suffix_begin(_name, _len)                                 \
    stars_names.suffix.list[it] = calloc(1,                             \
            sizeof(*stars_names.suffix.list[it]) +                      \
            sizeof(stars_names.suffix.list[it]->list[0]) * _len);       \
    stars_names.suffix.list[it]->len = _len;                            \
    stars_names.suffix.list[it]->name = make_symbol_len(_name, sizeof(_name));

#define stars_suffix(_ix, _name)                                        \
    stars_names.suffix.list[it]->list[_ix] = \
        make_symbol_len(_name, sizeof(_name));

#define stars_suffix_end()                              \
    stars_names.suffix.len = ++it;                      \
    assert(it < array_len(stars_names.suffix.list));

#include "gen/stars_suffix.h"

#undef stars_suffix_begin
#undef stars_suffix
#undef stars_suffix_end
    }
}


// -----------------------------------------------------------------------------
// rolls
// -----------------------------------------------------------------------------

static struct
{
    size_t range, len;
    struct stars_rolls *list[16];
} stars_rolls;


const struct stars_rolls *stars_rolls_pick(struct rng *rng)
{
    size_t value = rng_uni(rng, 0, stars_rolls.range);
    struct stars_rolls **it = stars_rolls.list;
    while (value > (*it)->weight) { value -= (*it)->weight; it++; }
    return *it;
}

static void stars_rolls_populate(void)
{
    size_t it = 0;

#define stars_rolls_begin( _name, _weight, _hue, _len)          \
    stars_rolls.list[it] = calloc(1,                            \
            sizeof(*stars_rolls.list[it]) +                     \
            sizeof(stars_rolls.list[it]->ranges[0]) * _len);    \
    *stars_rolls.list[it] = (struct stars_rolls) {              \
        .name = make_symbol_len(_name, sizeof(_name)),          \
        .weight = _weight,                                      \
        .hue = _hue,                                            \
        .len = _len,                                            \
    };

#define stars_rolls(_ix, _type, _min, _max, _count)                     \
    stars_rolls.list[it]->ranges[_ix] = (struct stars_roll_range) {     \
        .type = stars_roll_ ## _type,                                   \
        .min = _min,                                                    \
        .max = _max,                                                    \
        .count = _count,                                                \
    };

#define stars_rolls_end()                               \
    stars_rolls.range += stars_rolls.list[it]->weight;  \
    stars_rolls.len = ++it;                             \
    assert(it < array_len(stars_rolls.list));

#include "gen/stars_rolls.h"

#undef stars_rolls_begin
#undef stars_rolls
#undef stars_rolls_end

}


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

void stars_populate(void)
{
    stars_names_populate();
    stars_rolls_populate();
}
