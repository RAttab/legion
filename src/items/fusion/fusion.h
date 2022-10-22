/* fusion.h
   Rémi Attab (remi.attab@gmail.com), 15 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "db/items.h"
#include "db/specs.h"
#include "items/types.h"

struct im_config;


// -----------------------------------------------------------------------------
// fusion
// -----------------------------------------------------------------------------

struct legion_packed im_fusion
{
    im_id id;

    bool paused;
    bool waiting;
    im_energy energy;
};

static_assert(sizeof(struct im_fusion) == 8);

void im_fusion_config(struct im_config *);
