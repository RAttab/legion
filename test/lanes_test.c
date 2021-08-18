/* lanes_test.c
   Rémi Attab (remi.attab@gmail.com), 18 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/world.h"
#include "game/sector.h"
#include "game/chunk.h"
#include "game/lanes.h"
#include "utils/hset.h"

// -----------------------------------------------------------------------------
// checks
// -----------------------------------------------------------------------------

#define check_hset(val, ...)                            \
    do {                                                \
        struct hset *exp = make_hset(__VA_ARGS__);      \
        if (hset_eq(val, exp)) { free(exp); break; }    \
                                                        \
        char val_str[256];                              \
        hset_str(val, val_str, sizeof(val_str));        \
                                                        \
        char exp_str[256];                              \
        hset_str(exp, exp_str, sizeof(exp_str));        \
                                                        \
        dbg("%s != %s", val_str, exp_str);              \
        abort();                                        \
    } while (false)

#define check_hset_nil(val)                             \
    do {                                                \
        if (!val || !val->len) break;                   \
                                                        \
        char val_str[256];                              \
        hset_str(val, val_str, sizeof(val_str));        \
                                                        \
        dbg("%s != { }", val_str);                      \
        abort();                                        \
    } while (false)


// -----------------------------------------------------------------------------
// tests
// -----------------------------------------------------------------------------

void test_basics(void)
{
    struct world *world = world_new();
    struct sector *sector = world_sector(world, coord_center());

    enum item item = ITEM_LEGION_1;
    struct coord src = sector->stars[0].coord;
    struct coord dst = sector->stars[1].coord;

    for (size_t iteration = 0; iteration < 5; ++iteration) {
        world_lanes_launch(world, item, src, dst, NULL, 0);
        check_hset(world_lanes_list(world, src), coord_to_id(dst));
        check_hset(world_lanes_list(world, dst), coord_to_id(src));

        world_ts_delta_t wait = lanes_travel(item, src, dst);
        for (world_ts_delta_t i = 0; i < wait; ++i) world_step(world);

        check_hset_nil(world_lanes_list(world, src));
        check_hset_nil(world_lanes_list(world, dst));

        struct chunk *chunk = sector_chunk(sector, dst);
        assert(chunk);

        ssize_t ret = chunk_scan(chunk, ITEM_BRAIN_1);
        assert(ret >= 0);
        assert((size_t) ret > iteration);
    }
}


int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    test_basics();

    return 0;
}
