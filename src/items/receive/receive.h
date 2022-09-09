/* receive.h
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
// receive
// -----------------------------------------------------------------------------

struct legion_packed im_receive
{
    im_id id;

    uint8_t channel;
    uint8_t head, tail;

    legion_pad(3);

    struct coord target;

    struct im_packet buffer[];
};

static_assert(sizeof(struct im_receive) == 16);

size_t im_receive_cap(const struct im_receive *);
void im_receive_config(struct im_config *);
