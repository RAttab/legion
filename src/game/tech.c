/* tech.c
   Rémi Attab (remi.attab@gmail.com), 31 Oct 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/tech.h"
#include "game/save.h"

// -----------------------------------------------------------------------------
// tech
// -----------------------------------------------------------------------------

void tech_free(struct tech *tech)
{
    htable_reset(&tech->research);
}

void tech_save(const struct tech *tech, struct save *save)
{
    save_write_magic(save, save_magic_tech);

    tape_set_save(&tech->known, save);
    tape_set_save(&tech->learned, save);
    save_write_htable(save, &tech->research);

    save_write_magic(save, save_magic_tech);
}

bool tech_load(struct tech *tech, struct save *save)
{
    if (!save_read_magic(save, save_magic_tech)) return false;

    if (!tape_set_load(&tech->known, save)) return false;
    if (!tape_set_load(&tech->learned, save)) return false;
    if (!save_read_htable(save, &tech->research)) return false;

    return save_read_magic(save, save_magic_tech);
}

bool tech_known(const struct tech *tech, enum item item)
{
    assert(item);
    return tape_set_check(&tech->known, item);
}

struct tape_set tech_known_list(const struct tech *tech)
{
    return tech->known;
}

void tech_learn(struct tech *tech, enum item item)
{
    assert(item);
    tape_set_put(&tech->learned, item);

    struct tape_set todo = tape_set_invert(&tech->known);
    for (enum item it = tape_set_next(&todo, 0); it; it = tape_set_next(&todo, it)) {
        const struct tape_info *info = tapes_info(it);
        if (!info) continue;

        size_t intersect = tape_set_intersect(&tech->learned, &info->reqs);
        if (intersect == tape_set_len(&info->reqs))
            tape_set_put(&tech->known, it);
    }
}

bool tech_learned(const struct tech *tech, enum item item)
{
    assert(item);
    return tape_set_check(&tech->learned, item);
}

struct tape_set tech_learned_list(const struct tech *tech)
{
    return tech->learned;
}

uint64_t tech_learned_bits(const struct tech *tech, enum item item)
{
    assert(item);

    const uint8_t bits = im_config_assert(item)->lab_bits;
    const uint64_t mask = (1ULL << bits) - 1;
    if (tape_set_check(&tech->learned, item)) return mask;

    struct htable_ret ret = htable_get(&tech->research, item);
    return ret.ok ? ret.value : 0;
}

void tech_learn_bit(struct tech *tech, enum item item, uint8_t bit)
{
    const uint8_t bits = im_config_assert(item)->lab_bits;
    assert(bit < bits);

    if (tape_set_check(&tech->learned, item)) return;

    struct htable_ret ret = htable_get(&tech->research, item);
    uint64_t value = ret.ok ? ret.value : 0;
    value |= 1ULL << bit;

    const uint64_t mask = (1ULL << bits) - 1;
    if (value != mask) {
        if (ret.ok) ret = htable_xchg(&tech->research, item, value);
        else ret = htable_put(&tech->research, item, value);
        assert(ret.ok);
        return;
    }

    tech_learn(tech, item);
    ret = htable_del(&tech->research, item);
    assert(ret.ok);
}

void tech_populate(struct tech *tech)
{
    tape_set_put(&tech->known, ITEM_NIL);
    for (enum item it = ITEM_NATURAL_FIRST; it < ITEM_NATURAL_LAST; ++it)
        tape_set_put(&tech->known, it);

    // \todo Should not be part of the base set
    tape_set_put(&tech->known, ITEM_SCANNER);
    tape_set_union(&tech->known, &tapes_info(ITEM_SCANNER)->reqs);

    tape_set_put(&tech->known, ITEM_LEGION);
    tape_set_union(&tech->known, &tapes_info(ITEM_LEGION)->reqs);

    tape_set_put(&tech->known, ITEM_LAB);
    tape_set_union(&tech->known, &tapes_info(ITEM_LAB)->reqs);
}
