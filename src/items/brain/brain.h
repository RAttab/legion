/* brain.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "items/types.h"
#include "vm/vm.h"
#include "vm/mod.h"

struct im_config;


// -----------------------------------------------------------------------------
// brain
// -----------------------------------------------------------------------------

struct legion_packed im_brain
{
    id_t id;

    mod_t mod_id;
    const struct mod *mod;
    bool mod_fault;

    legion_pad(2);

    bool debug;
    ip_t breakpoint;

    legion_pad(8);

    struct im_packet msg;

    struct vm vm;
};

static_assert(sizeof(struct im_brain) == s_cache_line + sizeof(struct vm));

void im_brain_config(struct im_config *);
