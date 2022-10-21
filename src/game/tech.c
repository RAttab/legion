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
        if (intersect == tape_set_len(&info->reqs)) {
            tape_set_put(&tech->known, it);

            // We want to unlock elem-o whenever we unlock elem-m. Problem is
            // that elem-o doesn't have a tape which means that we need a
            // special shim for it. Not elegant but whatever.
            if (it == item_elem_m) tape_set_put(&tech->known, item_elem_o);
        }
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

    const uint8_t bits = specs_var_assert(make_spec(item, spec_lab_bits));
    const uint64_t mask = (1ULL << bits) - 1;
    if (tape_set_check(&tech->learned, item)) return mask;

    struct htable_ret ret = htable_get(&tech->research, item);
    return ret.ok ? ret.value : 0;
}

void tech_learn_bit(struct tech *tech, enum item item, uint8_t bit)
{
    const uint8_t bits = specs_var_assert(make_spec(item, spec_lab_bits));
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
    tape_set_put(&tech->known, item_nil);
    for (enum item it = items_natural_first; it < items_natural_last; ++it)
        tape_set_put(&tech->known, it);

    tape_set_put(&tech->known, item_rod);

    tape_set_put(&tech->known, item_legion);
    tape_set_union(&tech->known, &tapes_info(item_legion)->reqs);

    tape_set_put(&tech->known, item_lab);
    tape_set_union(&tech->known, &tapes_info(item_lab)->reqs);
}
