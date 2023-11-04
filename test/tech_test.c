/* test.c
   Rémi Attab (remi.attab@gmail.com), 24 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "db.h"
#include "vm.h"
#include "game.h"
#include "engine.h"


// -----------------------------------------------------------------------------
// tech
// -----------------------------------------------------------------------------

void check_learn_bits(struct tech *tech, enum item item)
{
    if (tech_learned(tech, item)) return;

    const uint8_t bits = specs_var_assert(make_spec(item, spec_lab_bits));
    for (uint8_t bit = 0; bit < bits; ++bit) {
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

    for (enum item it = tape_set_next(&info->tech, 0); it;
         it = tape_set_next(&info->tech, it))
    {
        if (!tech_learned(tech, it))
            assert(!tech_known(tech, item));
        check_learn_bits(tech, it);
    }

    assert(tech_known(tech, item));
}

void test_tech(void)
{
    struct tech tech = {0};
    tech_populate(&tech);

    for (enum item it = items_active_first; it < items_active_last; ++it)
        check_learn_item(&tech, it);
    for (enum item it = items_logistics_first; it < items_logistics_last; ++it)
        check_learn_item(&tech, it);

    tech_free(&tech);
}


// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

int main(int, char **)
{
    engine_populate_tests();

    test_tech();

    return 0;
}
