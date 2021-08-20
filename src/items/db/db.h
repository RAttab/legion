/* db.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "vm/vm.h"

struct im_config;


// -----------------------------------------------------------------------------
// db
// -----------------------------------------------------------------------------

struct legion_packed im_db
{
    id_t id;
    uint8_t len;
    legion_pad(3);
    word_t data[];
};

static_assert(sizeof(struct im_db) == 8);

void im_db_config(struct im_config *);
