/* storage.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "db/items.h"

struct im_config;

// -----------------------------------------------------------------------------
// storage
// -----------------------------------------------------------------------------

enum { im_storage_max = 4096 };

struct legion_packed im_storage
{
    im_id id;

    uint16_t count;
    enum item item;

    bool waiting;
};

static_assert(sizeof(struct im_storage) == 6);

void im_storage_config(struct im_config *);
