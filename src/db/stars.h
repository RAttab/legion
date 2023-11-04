/* stars.h
   Rémi Attab (remi.attab@gmail.com), 19 Jul 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct rng;


// -----------------------------------------------------------------------------
// stars_names
// -----------------------------------------------------------------------------

struct stars_names
{
    size_t len;
    struct symbol name;
    struct symbol list[];
};

const struct stars_names *stars_names_prefixes(void);
const struct stars_names *stars_names_suffixes(uint64_t rng);
const struct symbol *stars_names_pick(const struct stars_names *, uint64_t rng);


// -----------------------------------------------------------------------------
// rolls
// -----------------------------------------------------------------------------

enum stars_roll_type : uint8_t
{
    stars_roll_one,
    stars_roll_rng,
    stars_roll_one_of,
    stars_roll_all_of,
};

struct stars_roll_range
{
    enum stars_roll_type type;
    enum item min, max;
    uint16_t count;
};

struct stars_rolls
{
    struct symbol name;
    uint64_t weight;
    uint16_t hue;

    size_t len;
    struct stars_roll_range ranges[];
};

const struct stars_rolls *stars_rolls_pick(struct rng *);


// -----------------------------------------------------------------------------
// populate
// -----------------------------------------------------------------------------

void stars_populate(void);
