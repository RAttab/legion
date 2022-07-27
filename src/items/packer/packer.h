/* packer.h
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "items/item.h"
#include "items/types.h"

struct im_config;


// -----------------------------------------------------------------------------
// packer
// -----------------------------------------------------------------------------

struct legion_packed im_packer
{
    id_t id;
    enum item item;
    loops_t loops;
    bool waiting;
};

static_assert(sizeof(struct im_packer) == 8);

void im_packer_config(struct im_config *);
