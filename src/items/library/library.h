/* library.h
   Rémi Attab (remi.attab@gmail.com), 28 Jun 2023
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "vm/vm.h"

struct im_config;


// -----------------------------------------------------------------------------
// library
// -----------------------------------------------------------------------------

enum im_library_op : uint8_t
{
    im_library_nil = 0,
    im_library_in,
    im_library_out,
    im_library_tech,
};

struct legion_packed im_library
{
    im_id id;

    enum im_library_op op;
    enum item item, value;
    uint8_t index, len;

    legion_pad(1);
};

static_assert(sizeof(struct im_library) == 8);

void im_library_config(struct im_config *);
