/* tapes.c
   Rémi Attab (remi.attab@gmail.com), 19 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


// -----------------------------------------------------------------------------
// tape
// -----------------------------------------------------------------------------

static struct
{
    struct tape *index[items_max];
    struct tape_info *info[items_max];
} tapes;

const struct tape *tapes_get(enum item id)
{
    return id < items_max ? tapes.index[id] : NULL;
}

const struct tape_info *tapes_info(enum item id)
{
    return tapes_get(id) ? tapes.info[id] : NULL;
}

static void populate_tapes(void)
{
    const size_t len = 2 * s_page_len;
    void *it = mmap(0, len, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (it == MAP_FAILED) fail_errno("unable to mmap tapes");
    void *const end = it + len;


#define tape_register_begin(_item, _len)                        \
    {                                                           \
        struct tape *tape = tapes.index[_item] = it;            \
        it += sizeof(struct tape) + _len * sizeof(enum item);   \
        assert(it <= end);                                      \
        *tape = (struct tape)

#define tape_register_ix(_ix, _item)            \
    tape->tape[_ix] = _item

#define tape_register_end() }

#include "gen/tapes.h"

#undef tape_register_begin
#undef tape_register_end

}

static void populate_tapes_info(void)
{
    size_t len = sizeof(struct tape_info) * UINT8_MAX;
    void *it = mmap(0, len, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (it == MAP_FAILED) fail_errno("unable to mmap tapes info");
    void *const end = it + len;


#define tape_info_register_begin(_item)                         \
    {                                                           \
        struct tape_info *info = tapes.info[_item] = it;        \
        it += sizeof(struct tape_info);                         \
        assert(it <= end);                                      \
        *info = (struct tape_info)

#define tape_info_register_inputs(_req) \
    tape_set_put(&info->inputs, _req)

#define tape_info_register_tech(_req) \
    tape_set_put(&info->tech, _req)

#define tape_info_register_elems(_elem, _count) \
    info->elems[_elem] = _count

#define tape_info_register_end() }

#include "gen/tapes_info.h"

#undef tape_info_register_begin
#undef tape_info_register_end

}

void tapes_populate(void)
{
    populate_tapes();
    populate_tapes_info();
}
