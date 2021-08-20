/* brain.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "vm/vm.h"
#include "vm/mod.h"

struct im_config;


// -----------------------------------------------------------------------------
// brain
// -----------------------------------------------------------------------------

enum { im_brain_msg_cap = 4 };

struct legion_packed im_brain
{
    id_t id;

    ip_t breakpoint;
    bool debug;

    legion_pad(2);

    bool mod_fault;
    mod_t mod_id;
    const struct mod *mod;

    id_t msg_src;
    uint8_t msg_len;
    legion_pad(3);
    word_t msg[im_brain_msg_cap];

    struct vm vm;
};

static_assert(sizeof(struct im_brain) == s_cache_line + sizeof(struct vm));

void im_brain_config(struct im_config *);
