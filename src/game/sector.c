/* galaxy.c
   Rémi Attab (remi.attab@gmail.com), 12 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

bool star_load(struct star *star, struct save *save)
{
    if (!save_read_magic(save, save_magic_star)) return false;
    save_read_into(save, &star->coord);
    save_read_into(save, &star->hue);
    save_read_into(save, &star->energy);
    save_read_into(save, &star->elems);
    return save_read_magic(save, save_magic_star);
}

void star_save(const struct star *star, struct save *save)
{
    save_write_magic(save, save_magic_star);
    save_write_value(save, star->coord);
    save_write_value(save, star->hue);
    save_write_value(save, star->energy);
    save_write_from(save, &star->elems);
    save_write_magic(save, save_magic_star);
}


// -----------------------------------------------------------------------------
// sector
// -----------------------------------------------------------------------------

struct sector *sector_new(size_t stars)
{
    struct sector *sector = mem_struct_alloc_t(sector, sector->stars[0], stars);
    sector->stars_len = stars;
    htable_reserve(&sector->index, stars);

    return sector;
}

void sector_free(struct sector *sector)
{
    htable_reset(&sector->index);
    mem_free(sector);
}

const struct star *sector_star_in(
        const struct sector *sector, struct coord_rect rect)
{
    for (size_t i = 0; i < sector->stars_len; ++i) {
        const struct star *star = &sector->stars[i];
        if (coord_rect_contains(rect, star->coord)) return star;
    }
    return NULL;
}

const struct star *sector_star_at(
        const struct sector *sector, struct coord coord)
{
    struct htable_ret ret = htable_get(&sector->index, coord_to_u64(coord));
    return ret.ok ? (struct star *) ret.value : NULL;
}

ssize_t sector_scan(
        const struct sector *sector, struct coord coord, enum item item)
{
    uint64_t id = coord_to_u64(coord);

    struct htable_ret ret = htable_get(&sector->index, id);
    if (!ret.ok) return -1;

    struct star *star = (void *) ret.value;
    return star_scan(star, item);
}

// -----------------------------------------------------------------------------
// gen
// -----------------------------------------------------------------------------

constexpr size_t gen_stars_min = 4;
constexpr size_t gen_stars_max = 1000;

constexpr size_t gen_square_size = 256;
constexpr size_t gen_square_margin = 128;
constexpr size_t gen_square_fuzz = gen_square_size - gen_square_margin;

constexpr size_t gen_square_num = 256;
constexpr size_t gen_square_total = gen_square_num * gen_square_num;

constexpr size_t gen_prefix_cap = 15;
constexpr size_t gen_suffix_cap = 14;

static_assert(gen_square_size > gen_square_margin);
static_assert(gen_square_num * gen_square_num > gen_stars_max * 2);
static_assert(gen_square_num * gen_square_size == coord_sector_size);
static_assert(gen_square_total > 0);
static_assert(gen_prefix_cap + gen_suffix_cap + 1 == symbol_cap);


static uint64_t gen_mix(struct coord coord, world_seed seed)
{
    return coord_to_u64(coord) ^ seed;
}

static struct rng gen_rng(struct coord coord, world_seed seed)
{
    return rng_make(gen_mix(coord, seed));
}


// -----------------------------------------------------------------------------
// gen - name
// -----------------------------------------------------------------------------

struct name_bits { size_t prefix, suffix; };

struct name_bits name_gen_bits(struct coord coord, world_seed seed)
{
    uint64_t bits = hash_u64(gen_mix(coord, seed));
    return (struct name_bits) {
        .prefix = u64_top(bits),
        .suffix = u64_bot(bits),
    };
}

static struct symbol name_gen_symbol(
        const struct symbol *prefix,
        const struct symbol *suffix)
{
    assert(prefix->len + suffix->len + 1U < symbol_cap);

    struct symbol name = *prefix;
    name.c[name.len] = '-';
    name.len++;
    memcpy(name.c + name.len, suffix->c, suffix->len);
    name.len += suffix->len;
    return name;

}

struct symbol sector_name(struct coord coord, world_seed seed)
{
    struct name_bits bits = name_gen_bits(coord_sector(coord), seed);
    const struct stars_names *prefix = stars_names_prefixes();
    const struct stars_names *suffix = stars_names_suffixes(bits.suffix);

    return name_gen_symbol(
            stars_names_pick(prefix, bits.prefix),
            &suffix->name);
}

vm_word star_name(struct coord coord, world_seed seed, struct atoms *atoms)
{
    struct name_bits bits = {0};

    bits = name_gen_bits(coord_sector(coord), seed);
    const struct stars_names *prefix = stars_names_prefixes();
    const struct stars_names *suffix = stars_names_suffixes(bits.suffix);

    bits = name_gen_bits(coord, seed);
    struct symbol name = name_gen_symbol(
            stars_names_pick(prefix, bits.prefix),
            stars_names_pick(suffix, bits.suffix));

    return atoms_make(atoms, &name);
}


// -----------------------------------------------------------------------------
// gen - star
// -----------------------------------------------------------------------------

static void star_gen_inc(struct star *star, enum item item, uint16_t inc)
{
    uint16_t *ptr = NULL;
    if (item == item_energy) ptr = &star->energy;
    else if (item_is_natural(item))
        ptr = &star->elems[item - items_natural_first];
    else assert(false);

    *ptr = u16_saturate_add(*ptr, inc);
}

static bool star_gen_range(
        const struct stars_roll_range *range,
        struct star *star,
        struct rng *rng)
{
    const uint16_t max = range->count;
    const uint16_t min = max / 10;

    switch (range->type)
    {

    case stars_roll_one: {
        star_gen_inc(star, range->min, rng_uni(rng, min, max));
        return true;
    }

    case stars_roll_rng: {
        size_t n = rng_uni(rng, 0, range->max - range->min);
        for (size_t i = 0; i < n; ++i) {
            star_gen_inc(star,
                    rng_uni(rng, range->min, range->max + 1),
                    rng_uni(rng, min, max));
        }
        return true;
    }

    case stars_roll_one_of: {
        star_gen_inc(star,
                rng_uni(rng, range->min, range->max + 1),
                rng_uni(rng, min, max));
        return true;
    }

    case stars_roll_all_of: {
        for (enum item it = range->min; it <= range->max; ++it)
            star_gen_inc(star, it, rng_uni(rng, min, max));
        return true;
    }

    default: { assert(false); }
    }
}

void star_gen(struct star *star, struct coord coord, world_seed seed)
{
    struct rng rng = gen_rng(coord, seed);
    star->coord = coord;

    const struct stars_rolls *rolls = stars_rolls_pick(&rng);
    for (size_t i = 0; i < rolls->len; ++i)
        star_gen_range(rolls->ranges + i, star, &rng);

    int16_t hue = rolls->hue + (15 - rng_uni(&rng, 0, 30));
    star->hue = hue < 0 ? 360 + hue : hue;
}


// -----------------------------------------------------------------------------
// gen - sector
// -----------------------------------------------------------------------------

// Very elaborate way of avoiding star overlap without having to constantly look
// for collisions.
struct vec64 *sector_gen_stars(struct rng *rng, struct coord base, size_t stars)
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

struct sector *sector_gen(struct coord coord, world_seed seed)
{
    coord = coord_sector(coord);
    struct rng rng = gen_rng(coord, seed);

    uint64_t mid = coord_mid;
    uint128_t delta_max = mid * mid;
    uint128_t delta = coord_dist_2(coord, coord_center());

    size_t stars = gen_stars_min;
    if (delta < delta_max)
        stars = (gen_stars_max * (delta_max - delta)) / delta_max;

    struct vec64 *list = sector_gen_stars(&rng, coord, stars);
    struct sector *sector = sector_new(vec64_len(list));
    sector->coord = coord;

    for (size_t i = 0; i < sector->stars_len; ++i) {
        struct star *star = &sector->stars[i];

        struct coord coord_star = coord_from_u64(list->vals[i]);
        star_gen(star, coord_star, seed);

        struct htable_ret ret =
            htable_put(&sector->index, coord_to_u64(coord_star), (uintptr_t) star);
        if (!ret.ok) continue;
    }

    vec64_free(list);
    return sector;
}
