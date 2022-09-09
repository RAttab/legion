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
    im_id id;

    struct { work left, cap; } work;
    enum item item;

    legion_pad(3);

    struct coord coord;
    vm_word result;

};

static_assert(sizeof(struct im_prober) == 24);

void im_prober_config(struct im_config *);
