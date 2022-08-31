/* items_test.c
   Rémi Attab (remi.attab@gmail.com), 28 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/world.h"
#include "game/sector.h"
#include "game/chunk.h"
#include "game/lanes.h"
#include "game/sys.h"
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

void wait_travel(
        struct world *world, size_t speed, struct coord src, struct coord dst)
{
    world_ts_delta_t wait = lanes_travel(speed, src, dst);
    for (world_ts_delta_t i = 0; i < wait; ++i) world_step(world);
}

void step_for(struct world *world, world_ts_t ticks)
{
    for (world_ts_t ts = 0; ts < ticks; ++ts) world_step(world);
}

size_t storage_count(struct chunk *chunk, id_t storage_id, id_t test_id)
{
    const word_t io_loop = IO_LOOP;
    const struct im_test *test = chunk_get(chunk, test_id);

    chunk_io(chunk, IO_STATE, test_id, storage_id, &io_loop, 1);
    assert(test->io == IO_RETURN);
    assert(test->src == storage_id);
    assert(test->len == 1);

    return test->args[0];
}


// -----------------------------------------------------------------------------
// impl
// -----------------------------------------------------------------------------

#include "items/txrx_test.c"
#include "items/storage_test.c"
#include "items/port_test.c"


// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

int main(int, char **)
{
    sys_populate_tests();

    test_txrx();
    test_storage();
    test_port();

    return 0;
}
