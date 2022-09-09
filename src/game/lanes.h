/* lanes.h
   Rémi Attab (remi.attab@gmail.com), 03 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/coord.h"
#include "game/world.h"
#include "utils/heap.h"

struct hset;


// -----------------------------------------------------------------------------
// lanes
// -----------------------------------------------------------------------------

struct lanes
{
    struct world *world;

    struct htable lanes;
    struct htable index;
    struct heap data;
};

void lanes_init(struct lanes *, struct world *);
void lanes_free(struct lanes *);

bool lanes_load(struct lanes *, struct world *, struct save *);
void lanes_save(struct lanes *, struct save *);

world_ts_delta lanes_travel(size_t speed, struct coord src, struct coord dst);

const struct hset *lanes_list(struct lanes *, struct coord key);
void lanes_list_save(struct lanes *, struct save *, struct world *, user_set);
bool lanes_list_load_into(struct htable *, struct save *);

void lanes_launch(
        struct lanes *,
        user_id owner, enum item type, size_t speed,
        struct coord src, struct coord dst,
        const vm_word *data, size_t len);

void lanes_step(struct lanes *);
