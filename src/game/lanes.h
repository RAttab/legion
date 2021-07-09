/* lanes.h
   Rémi Attab (remi.attab@gmail.com), 03 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/coord.h"
#include "game/world.h"

struct vec64;

// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

struct lanes
{
    struct world *world;

    struct htable lanes;
    struct htable index;
};

void lanes_init(struct lanes *, struct world *);
void lanes_free(struct lanes *);

bool lanes_load(struct lanes *, struct world *, struct save *);
void lanes_save(struct lanes *, struct save *);

struct vec64 *lanes_list(struct lanes *, struct coord key);

void lanes_launch(struct lanes *,
        struct coord src, struct coord dst, enum item type, uint32_t data);

void lanes_step(struct lanes *);
