/* galaxy.h
   Rémi Attab (remi.attab@gmail.com), 12 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/save.h"
#include "items/item.h"
#include "game/coord.h"
#include "vm/vm.h"
#include "utils/htable.h"

struct chunk;

// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

enum legion_packed star_state
{
    star_untouched,
    star_active,
    star_barren,
};

struct legion_packed star
{
    struct coord coord;
    uint16_t power;
    enum star_state state;
    legion_pad(1);
    uint16_t elems[ITEMS_NATURAL_LEN];
};
static_assert(sizeof(struct star) == 4 * 8 + 2);

void star_gen(struct star *, struct coord);
bool star_load(struct star *, struct save *);
void star_save(struct star *, struct save *);

inline uint16_t star_elem(struct star *star, enum item item)
{
    assert(item >= ITEM_NATURAL_FIRST && item < ITEM_NATURAL_LAST);
    return star->elems[item - ITEM_NATURAL_FIRST];
}

// -----------------------------------------------------------------------------
// sector
// -----------------------------------------------------------------------------

struct sector
{
    struct world *world;

    struct coord coord;
    struct htable chunks;
    struct htable index;

    size_t stars_len;
    struct star stars[];
};

struct sector *sector_gen(struct world *, struct coord coord);
void sector_free(struct sector *);

struct sector *sector_load(struct world *, struct save *);
void sector_save(struct sector *, struct save *);

struct chunk *sector_chunk(struct sector *, struct coord coord);
struct chunk *sector_chunk_alloc(struct sector *, struct coord coord);

const struct star *sector_star_in(struct sector *, struct rect);
const struct star *sector_star_at(struct sector *, struct coord coord);

void sector_step(struct sector *);
ssize_t sector_scan(struct sector *, struct coord, enum item);

void sector_lanes_arrive(
        struct sector *,
        enum item type,
        struct coord dst,
        const word_t *data, size_t len);
