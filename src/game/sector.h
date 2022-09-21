/* galaxy.h
   Rémi Attab (remi.attab@gmail.com), 12 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/save.h"
#include "items/item.h"
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
    uint16_t elems[ITEMS_NATURAL_LEN];
};
static_assert(sizeof(struct star) == 5 * 8);

bool star_load(struct star *, struct save *);
void star_save(const struct star *, struct save *);

inline uint16_t star_scan(const struct star *star, enum item item)
{
    if (item == ITEM_ENERGY) return star->energy;
    if (item >= ITEM_NATURAL_FIRST && item < ITEM_NATURAL_LAST)
        return star->elems[item - ITEM_NATURAL_FIRST];
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
