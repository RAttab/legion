/* storage.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "items/item.h"

struct im_config;

// -----------------------------------------------------------------------------
// storage
// -----------------------------------------------------------------------------

enum { im_storage_max = UINT8_MAX };

struct legion_packed im_storage
{
    id_t id;

    enum item item;
    uint8_t count;
    bool waiting;

    legion_pad(1);
};

static_assert(sizeof(struct im_storage) == 6);

void im_storage_config(struct im_config *);
