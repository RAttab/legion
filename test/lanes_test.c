/* lanes_test.c
   Rémi Attab (remi.attab@gmail.com), 18 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game.h"
#include "engine.h"

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
        dbgf("%s != %s", val_str, exp_str);             \
        abort();                                        \
    } while (false)

#define check_hset_nil(val)                             \
    do {                                                \
        if (!val || !val->len) break;                   \
                                                        \
        char val_str[256];                              \
        hset_str(val, val_str, sizeof(val_str));        \
                                                        \
        dbgf("%s != { }", val_str);                     \
        abort();                                        \
    } while (false)


void wait(struct world *world, size_t speed, struct coord src, struct coord dst)
{
    world_ts_delta wait = lanes_travel(speed, src, dst);
    for (world_ts_delta i = 0; i < wait; ++i) world_step(world);
}


// -----------------------------------------------------------------------------
// tests
// -----------------------------------------------------------------------------

void test_basics(void)
{
    struct world *world = world_new(0);
    const struct sector *sector = world_sector(world, coord_center());

    const user_id user = 13;
    const size_t speed = 100;
    const enum item item = item_pill;
    const struct coord src = sector->stars[0].coord;
    const struct coord dst = sector->stars[1].coord;

    for (size_t iteration = 0; iteration < 5; ++iteration) {
        world_lanes_launch(world, user, item, speed, src, dst, NULL, 0);
        check_hset(world_lanes_list(world, src), coord_to_u64(dst));
        check_hset(world_lanes_list(world, dst), coord_to_u64(src));

        world_lanes_launch(world, user, item, speed, dst, src, NULL, 0);
        check_hset(world_lanes_list(world, src), coord_to_u64(dst));
        check_hset(world_lanes_list(world, dst), coord_to_u64(src));

        wait(world, speed, src, dst);

        check_hset_nil(world_lanes_list(world, src));
        check_hset_nil(world_lanes_list(world, dst));

        struct chunk *chunk_src = world_chunk(world, src);
        struct chunk *chunk_dst = world_chunk(world, dst);
        assert(chunk_src && chunk_dst);

        assert(chunk_scan(chunk_src, item) > (ssize_t) iteration);
        assert(chunk_scan(chunk_dst, item) > (ssize_t) iteration);
        assert(chunk_owner(chunk_dst) == user);
    }

    world_free(world);
}

void test_speed(void)
{
    struct world *world = world_new(0);
    const struct sector *sector = world_sector(world, coord_center());

    enum { count = 10 };
    const size_t speed_slow = 10;
    const size_t speed_fast = speed_slow * 100;
    const enum item item_slow = item_pill;
    const enum item item_fast = item_memory;
    const struct coord src = sector->stars[0].coord;
    const struct coord dst = sector->stars[1].coord;
    struct chunk *chunk_dst = world_chunk_alloc(world, dst, user_admin);

    for (size_t i = 0; i < count; ++i) {
        world_lanes_launch(world, user_admin, item_slow, speed_slow, src, dst, NULL, 0);
        world_lanes_launch(world, user_admin, item_fast, speed_fast, src, dst, NULL, 0);
        world_step(world);
    }

    assert(chunk_scan(chunk_dst, item_slow) == 0);
    assert(chunk_scan(chunk_dst, item_fast) == 0);

    wait(world, speed_fast, src, dst);

    assert(chunk_scan(chunk_dst, item_slow) == 0);
    assert(chunk_scan(chunk_dst, item_fast) == count);

    wait(world, speed_slow, src, dst);

    assert(chunk_scan(chunk_dst, item_slow) == count);
    assert(chunk_scan(chunk_dst, item_fast) == count);

    world_free(world);
}


int main(int argc, char **argv)
{
    (void) argc, (void) argv;

    engine_populate_tests();

    test_basics();
    test_speed();

    return 0;
}
