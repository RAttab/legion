/* world_test.c
   Rémi Attab (remi.attab@gmail.com), 24 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/mod.h"
#include "game/tape.h"
#include "game/world.h"
#include "render/core.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// lab
// -----------------------------------------------------------------------------

void check_learn_bits(struct world *world, enum item item)
{
    if (world_lab_learned(world, item)) return;

    const struct im_config *config = im_config_assert(item);
    assert(config);

    for (uint8_t bit = 0; bit < config->lab_bits; ++bit) {
        assert(!world_lab_learned(world, item));
        world_lab_learn_bit(world, item, bit);
    }
    assert(world_lab_learned(world, item));
}

void check_learn_item(struct world *world, enum item item)
{
    if (world_lab_known(world, item)) return;

    const struct tape_info *info = tapes_info(item);
    if (!info) return;

    for (enum item it = tape_set_next(&info->reqs, 0); it;
         it = tape_set_next(&info->reqs, it))
    {
        assert(!world_lab_known(world, item));
        check_learn_bits(world, it);
    }

    assert(world_lab_known(world, item));
}

void test_lab(void)
{
    struct world *world = world_new(0);
    (void) world_populate(world);

    for (enum item it = ITEM_ACTIVE_FIRST; it < ITEM_ACTIVE_LAST; ++it)
        check_learn_item(world, it);
    for (enum item it = ITEM_LOGISTICS_FIRST; it < ITEM_LOGISTICS_LAST; ++it)
        check_learn_item(world, it);

    world_free(world);
}


// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

int main(int, char **)
{
    core_populate();

    test_lab();

    return 0;
}
