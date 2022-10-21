/* deploy.h
   Rémi Attab (remi.attab@gmail.com), 19 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "db/items.h"
#include "items/types.h"

struct im_config;


// -----------------------------------------------------------------------------
// deploy
// -----------------------------------------------------------------------------

struct legion_packed im_deploy
{
    im_id id;

    enum item item;
    im_loops loops;
    bool waiting;

    legion_pad(3);
};

static_assert(sizeof(struct im_deploy) == 8);

void im_deploy_config(struct im_config *);
