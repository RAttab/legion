/* prober.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "vm/vm.h"
#include "game/id.h"
#include "game/coord.h"
#include "game/world.h"
#include "items/item.h"

struct im_config;


// -----------------------------------------------------------------------------
// scan
// -----------------------------------------------------------------------------

struct legion_packed im_prober
{
    id_t id;

    struct { uint8_t left; uint8_t cap; } work;
    enum item item;

    legion_pad(3);

    struct coord coord;
    word_t result;

};

static_assert(sizeof(struct im_prober) == 24);

void im_prober_config(struct im_config *);
