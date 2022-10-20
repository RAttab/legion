/* tapes.c
   Rémi Attab (remi.attab@gmail.com), 19 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "tapes.h"

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
        if (id >= ITEM_NATURAL_FIRST && id < ITEM_SYNTH_FIRST)
            info->elems[id - ITEM_NATURAL_FIRST] = 1;
        return info;
    }

    for (size_t i = 0; i < tape->inputs; ++i) {
        const struct tape_info *input = tapes_info_for(tape->tape[i]);
        info->rank = legion_max(info->rank, input->rank + 1);

        for (size_t i = 0; i < ITEMS_NATURAL_LEN; ++i)
            info->elems[i] += input->elems[i];

        tape_set_union(&info->reqs, &input->reqs);
        tape_set_put(&info->reqs, tape->tape[i]);
    }

    return info;
}


void tapes_populate(void)
{

#define tape_register_begin(_item, _len)                                \
    {                                                                   \
        struct tape **tape = &(tapes.index[_item]);                     \
        *tape = malloc(sizeof(struct tape) + _len * sizeof(enum item)); \
        **tape = (struct tape)
    
#define tape_register_ix(_ix, _item) \
    (*tape)->tape[_ix] = _item

#define tape_register_end() }

    #include "gen/tapes.h"

#undef tape_register_begin
#undef tape_register_end

    for (enum item item = 0; item < ITEM_MAX; ++item)
        (void) tapes_info_for(item);
}
