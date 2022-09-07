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

    bool fault;
    bool debug;
    ip_t breakpoint;

    mod_t mod_id;
    legion_pad(12);
    const struct mod *mod;

    struct im_packet msg;

    struct vm vm;
};

static_assert(sizeof(struct im_brain) == s_cache_line + sizeof(struct vm));

void im_brain_config(struct im_config *);
