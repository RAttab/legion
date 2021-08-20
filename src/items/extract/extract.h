/* extract.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "game/tape.h"
#include "items/types.h"

struct im_config;


// -----------------------------------------------------------------------------
// extract
// -----------------------------------------------------------------------------

struct legion_packed im_extract
{
    id_t id;
    loops_t loops;
    bool waiting;
    legion_pad(1);
    tape_packed_t tape;
};

static_assert(sizeof(struct im_extract) == 16);

void im_extract_config(struct im_config *);
