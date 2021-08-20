/* legion.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "vm/mod.h"

struct im_config;


// -----------------------------------------------------------------------------
// legion
// -----------------------------------------------------------------------------

struct legion_packed im_legion
{
    id_t id;
    mod_t mod;
};

static_assert(sizeof(struct im_legion) == 8);

const enum item *im_legion_cargo(enum item type);

void im_legion_config(struct im_config *);
