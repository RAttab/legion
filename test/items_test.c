/* items_test.c
   Rémi Attab (remi.attab@gmail.com), 28 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/world.h"
#include "game/sector.h"
#include "game/chunk.h"
#include "game/lanes.h"
#include "render/core.h"
#include "items/io.h"
#include "items/item.h"
#include "items/types.h"
#include "items/config.h"
#include "items/test/test.h"
#include "vm/mod.h"
#include "vm/vm.h"


// -----------------------------------------------------------------------------
// utils
// -----------------------------------------------------------------------------

void wait(struct world *world, size_t speed, struct coord src, struct coord dst)
{
    world_ts_delta_t wait = lanes_travel(speed, src, dst);
    for (world_ts_delta_t i = 0; i < wait; ++i) world_step(world);
}


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/txrx.c"


// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

int main(int, char **)
{
    core_populate();

    test_txrx();

    return 0;
}
