/* tapes.h
   Rémi Attab (remi.attab@gmail.com), 20 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "db/items.h"
#include "game/types.h"

// -----------------------------------------------------------------------------
// tapes
// -----------------------------------------------------------------------------

void tapes_populate(void);
const struct tape *tapes_get(enum item id);

struct legion_packed tape_info
{
    im_rank rank;
    struct tape_set tech, inputs;
    uint32_t elems[items_natural_last];
};
const struct tape_info *tapes_info(enum item id);

