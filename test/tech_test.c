/* test.c
   Rémi Attab (remi.attab@gmail.com), 24 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "vm/mod.h"
#include "game/tape.h"
#include "game/tech.h"
#include "game/sys.h"
#include "items/config.h"


// -----------------------------------------------------------------------------
// tech
// -----------------------------------------------------------------------------

void check_learn_bits(struct tech *tech, enum item item)
{
    if (tech_learned(tech, item)) return;

    const struct im_config *config = im_config_assert(item);
    assert(config);

    for (uint8_t bit = 0; bit < config->lab_bits; ++bit) {
        assert(!tech_learned(tech, item));
        tech_learn_bit(tech, item, bit);
    }
    assert(tech_learned(tech, item));
}

void check_learn_item(struct tech *tech, enum item item)
{
    if (tech_known(tech, item)) return;

    const struct tape_info *info = tapes_info(item);
    if (!info) return;

    for (enum item it = tape_set_next(&info->reqs, 0); it;
         it = tape_set_next(&info->reqs, it))
    {
        if (!tech_known(tech, it))
            assert(!tech_known(tech, item));
        check_learn_bits(tech, it);
    }

    assert(tech_known(tech, item));
}

void test_tech(void)
{
    struct tech tech = {0};
    tech_populate(&tech);

    for (enum item it = ITEM_ACTIVE_FIRST; it < ITEM_ACTIVE_LAST; ++it)
        check_learn_item(&tech, it);
    for (enum item it = ITEM_LOGISTICS_FIRST; it < ITEM_LOGISTICS_LAST; ++it)
        check_learn_item(&tech, it);

    tech_free(&tech);
}


// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

int main(int, char **)
{
    sys_populate_tests();

    test_tech();

    return 0;
}
