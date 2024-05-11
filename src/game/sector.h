/* sector.h
   Rémi Attab (remi.attab@gmail.com), 12 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct save;
struct chunk;

// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

constexpr size_t star_elem_cap = UINT16_MAX;

// Number of pixels per star at scale_base (not map_scale_default). Basically it
// needs to be tweaked to a number that's big enough to see and click on but not
// too big that there are overlaps between stars during gen.
constexpr size_t star_size_cap = 1000;
constexpr size_t star_size_dec = 600;

struct legion_packed star
{
    struct coord coord;

    struct { uint16_t center, edge; } hue;
    uint16_t size;
    legion_pad(2);

    uint16_t energy;
    uint16_t elems[items_natural_len];
};
static_assert(sizeof(struct star) == 5 * 8);

vm_word star_name(struct coord, world_seed, struct atoms *);
struct sector *sector_gen(struct coord, world_seed);

bool star_load(struct star *, struct save *);
void star_save(const struct star *, struct save *);

inline uint16_t star_scan(const struct star *star, enum item item)
{
    if (item == item_energy) return star->energy;
    if (item >= items_natural_first && item < items_natural_last)
        return star->elems[item - items_natural_first];
    return 0;
}

bool star_hover(const struct star *, struct coord coord);

// -----------------------------------------------------------------------------
// sector
// -----------------------------------------------------------------------------

struct sector
{
    struct coord coord;
    struct htable index;

    size_t stars_len;
    struct star stars[];
};

struct symbol sector_name(struct coord, world_seed);
struct sector *sector_gen(struct coord, world_seed);

struct sector *sector_new(size_t stars);
void sector_free(struct sector *);

const struct star *sector_star_at(const struct sector *, struct coord);
const struct star *sector_star_find(const struct sector *, struct coord);

ssize_t sector_scan(const struct sector *, struct coord, enum item);
