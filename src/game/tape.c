/* tape.c
   Rémi Attab (remi.attab@gmail.com), 02 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/tape.h"
#include "game/sys.h"
#include "utils/fs.h"
#include "utils/str.h"
#include "utils/htable.h"
#include "utils/config.h"


// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

enum item tape_id(const struct tape *tape) { return tape->id; }
enum item tape_host(const struct tape *tape) { return tape->host; }
im_energy tape_energy(const struct tape *tape) { return tape->energy; }
im_work tape_work_cap(const struct tape *tape) { return tape->work; }

size_t tape_len(const struct tape *tape)
{
    return tape->inputs + tape->work + tape->outputs;
}

enum item tape_input_at(const struct tape *tape, tape_it it)
{
    return it < tape->inputs ? tape->tape[it] : item_nil;
}

enum item tape_output_at(const struct tape *tape, tape_it it)
{
    return it < tape->outputs ? tape->tape[tape->inputs + it] : item_nil;
}

struct tape_ret tape_at(const struct tape *tape, tape_it it)
{
    tape_it bound = tape->inputs;
    if (it < bound) {
        return (struct tape_ret) {
            .state = tape_input,
            .item = tape->tape[it],
        };
    }

    bound += tape->work;
    if (it < bound) {
        return (struct tape_ret) {
            .state = tape_work,
            .item = item_nil,
        };
    }

    bound += tape->outputs;
    if (it < bound) {
        return (struct tape_ret) {
            .state = tape_output,
            .item = tape->tape[it - tape->work],
        };
    }

    return (struct tape_ret) {
        .state = tape_eof,
        .item = item_nil,
    };
}


// -----------------------------------------------------------------------------
// tape_set
// -----------------------------------------------------------------------------

size_t tape_set_len(const struct tape_set *set)
{
    size_t n = 0;
    for (size_t i = 0; i < array_len(set->s); ++i)
        n += u64_pop(set->s[i]);
    return n;
}

bool tape_set_empty(const struct tape_set *set)
{
    for (size_t i = 0; i < array_len(set->s); ++i)
        if (set->s[i]) return false;
    return true;
}

bool tape_set_check(const struct tape_set *set, enum item item)
{
    return set->s[item / 64] & (1ULL << (item % 64));
}

void tape_set_put(struct tape_set *set, enum item item)
{
    set->s[item / 64] |= 1ULL << (item % 64);
}

struct tape_set tape_set_invert(struct tape_set *set)
{
    struct tape_set ret = {0};
    for (size_t i = 0; i < array_len(set->s); ++i)
        ret.s[i] = ~set->s[i];
    return ret;
}

enum item tape_set_next(const struct tape_set *set, enum item first)
{
    if (first == items_max) return item_nil;
    first++;

    uint64_t mask = (1ULL << (first % 64)) - 1;
    for (size_t i = first / 64; i < array_len(set->s); ++i) {
        uint64_t x = set->s[i] & ~mask;
        if (x) return u64_ctz(x) + i * 64;
        mask = 0;
    }
    return item_nil;
}

enum item tape_set_at(const struct tape_set *set, size_t index)
{
    for (enum item item = tape_set_next(set, 0);
         item != item_nil; item = tape_set_next(set, item), --index) {
        if (!index) return item;
    }
    return item_nil;
}

bool tape_set_eq(const struct tape_set *lhs, const struct tape_set *rhs)
{
    for (size_t i = 0; i < array_len(lhs->s); ++i)
        if (lhs->s[i] != rhs->s[i]) return false;
    return true;
}

void tape_set_union(struct tape_set *set, const struct tape_set *other)
{
    for (size_t i = 0; i < array_len(set->s); ++i)
        set->s[i] |= other->s[i];
}

size_t tape_set_intersect(
        const struct tape_set *set, const struct tape_set *other)
{
    size_t n = 0;
    for (size_t i = 0; i < array_len(set->s); ++i)
        n += u64_pop(set->s[i] & other->s[i]);
    return n;
}

void tape_set_save(const struct tape_set *set, struct save *save)
{
    save_write_magic(save, save_magic_tape_set);
    save_write(save, set, sizeof(*set));
    save_write_magic(save, save_magic_tape_set);
}

bool tape_set_load(struct tape_set *set, struct save *save)
{
    if (!save_read_magic(save, save_magic_tape_set)) return false;
    save_read(save, set, sizeof(*set));
    return save_read_magic(save, save_magic_tape_set);
}
