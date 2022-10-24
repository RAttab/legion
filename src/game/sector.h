/* galaxy.h
   Rémi Attab (remi.attab@gmail.com), 12 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "utils/save.h"
#include "db/items.h"
#include "game/coord.h"
#include "game/world.h"
#include "vm/vm.h"
#include "utils/htable.h"

struct chunk;

// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

enum { star_elem_cap = UINT16_MAX };

struct legion_packed star
{
    struct coord coord;

    uint16_t hue;
    legion_pad(6);

    uint16_t energy;
    uint16_t elems[items_natural_len];
};
static_assert(sizeof(struct star) == 5 * 8);

bool star_load(struct star *, struct save *);
void star_save(const struct star *, struct save *);

inline uint16_t star_scan(const struct star *star, enum item item)
{
    if (item == item_energy) return star->energy;
    if (item >= items_natural_first && item < items_natural_last)
        return star->elems[item - items_natural_first];
    return 0;
}


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

struct sector *sector_gen(struct coord, world_seed);
struct sector *sector_new(size_t stars);
void sector_free(struct sector *);

struct sector *sector_load(struct world *, struct save *);
void sector_save(const struct sector *, struct save *);

const struct star *sector_star_in(const struct sector *, struct rect);
const struct star *sector_star_at(const struct sector *, struct coord coord);

ssize_t sector_scan(const struct sector *, struct coord, enum item);
