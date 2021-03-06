/* galaxy.h
   Rémi Attab (remi.attab@gmail.com), 12 Dec 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/item.h"
#include "game/coord.h"
#include "utils/htable.h"

struct hunk;

// -----------------------------------------------------------------------------
// star
// -----------------------------------------------------------------------------

enum star_state
{
    star_untouched,
    star_active,
    star_barren,
};

struct legion_packed star
{
    struct coord coord;
    enum star_state state;

    uint32_t power;
    uint16_t elements[elem_natural_len];

    legion_pad(16);
};
static_assert(sizeof(struct star) == s_cache_line);

void star_gen(struct star *star, struct coord coord);


// -----------------------------------------------------------------------------
// sector
// -----------------------------------------------------------------------------

struct sector
{
    struct coord coord;
    struct htable hunks;
    struct htable index;

    size_t stars_len;
    struct star stars[];
};

struct sector *sector_gen(struct coord coord);
void sector_preload(struct sector *);

struct hunk *sector_hunk(struct sector *, struct coord coord);
struct hunk *sector_hunk_alloc(struct sector *, struct coord coord);

const struct star *sector_star(struct sector *, const struct rect *);

void sector_step(struct sector *);
