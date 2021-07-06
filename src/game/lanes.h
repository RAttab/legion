/* lanes.h
   Rémi Attab (remi.attab@gmail.com), 03 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/coord.h"
#include "game/world.h"


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

struct legion_packed cargo
{
    world_ts_t ts;
    enum item type;
    enum item cargo;
    uint8_t count;
    legion_pad(1);
    struct coord dst;
};
static_assert(sizeof(struct cargo) == 16);

struct lanes
{
    struct world *world;

    size_t len, cap;
    struct cargo *data;
};

void lanes_init(struct lanes *, struct world *);
void lanes_free(struct lanes *);

bool lanes_load(struct lanes *, struct world *, struct save *);
void lanes_save(struct lanes *, struct save *);

void lanes_launch(
        struct lanes *,
        struct coord src, struct coord dst,
        enum item type, enum item cargo, uint8_t count);

void lanes_step(struct lanes *);
