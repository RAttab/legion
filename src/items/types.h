/* types.h
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/tape.h"
#include "items/item.h"


// -----------------------------------------------------------------------------
// loops
// -----------------------------------------------------------------------------

typedef uint16_t loops_t;
enum { loops_inf = UINT16_MAX };
inline loops_t loops_io(word_t loops)
{
    return loops > 0 && loops < loops_inf ? loops : loops_inf;
}


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

// used in config->gm.flow

struct legion_packed flow
{
    id_t id;
    uint16_t row, col;

    loops_t loops;
    enum item target;

    legion_pad(1);

    enum item in, out;
    tape_it_t tape_it, tape_len;
};

static_assert(sizeof(struct flow) == 16);
