/* transmit.h
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "items/item.h"
#include "items/types.h"

struct im_config;


// -----------------------------------------------------------------------------
// transmit
// -----------------------------------------------------------------------------

enum { im_transmit_speed = 500 };

struct legion_packed im_transmit
{
    id id;

    legion_pad(1);

    uint8_t channel;
    struct coord target;
};

static_assert(sizeof(struct im_transmit) == 12);

void im_transmit_config(struct im_config *);
