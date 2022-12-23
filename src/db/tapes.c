/* tapes.c
   Rémi Attab (remi.attab@gmail.com), 19 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "tapes.h"

#include <sys/mman.h>


// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

static struct
{
    struct tape *index[items_max];
    struct tape_info info[items_max];
} tapes;

const struct tape *tapes_get(enum item id)
{
    return id < items_max ? tapes.index[id] : NULL;
}

const struct tape_info *tapes_info(enum item id)
{
    return tapes_get(id) ? &tapes.info[id] : NULL;
}

static const struct tape_info *tapes_info_for(enum item id)
{
    const struct tape *tape = tapes_get(id);
    if (!tape) return NULL;

    struct tape_info *info = &tapes.info[id];
    if (info->rank) return info;

    if (!tape->inputs) {
        info->rank = 1;
        if (id >= items_natural_first && id < items_synth_first)
            info->elems[id - items_natural_first] = 1;
        return info;
    }

    info->energy = tape->energy * tape->work;

    for (size_t i = 0; i < tape->inputs; ++i) {
        const struct tape_info *input = tapes_info_for(tape->tape[i]);
        info->rank = legion_max(info->rank, input->rank + 1);

        info->energy += input->energy;
        for (size_t i = 0; i < items_natural_len; ++i)
            info->elems[i] += input->elems[i];

        tape_set_union(&info->reqs, &input->reqs);
        tape_set_put(&info->reqs, tape->tape[i]);
    }

    return info;
}


void tapes_populate(void)
{
    const size_t len = 2 * s_page_len;
    void *it = mmap(0, len, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (it == MAP_FAILED) fail_errno("unable to mmap tapes");
    void *const end = it + len;


#define tape_register_begin(_item, _len)                                \
    {                                                                   \
        struct tape *tape = tapes.index[_item] = it;                    \
        it += sizeof(struct tape) + _len * sizeof(enum item);           \
        assert(it <= end);                                              \
        *tape = (struct tape)

#define tape_register_ix(_ix, _item) \
    tape->tape[_ix] = _item

#define tape_register_end() }

    #include "gen/tapes.h"

#undef tape_register_begin
#undef tape_register_end


    for (enum item item = 0; item < items_max; ++item)
        (void) tapes_info_for(item);
}
