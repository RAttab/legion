/* printer.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "game/tape.h"
#include "items/types.h"

struct im_config;


// -----------------------------------------------------------------------------
// printer
// -----------------------------------------------------------------------------
// Also used for assembler. It's the exact same logic.

struct legion_packed im_printer
{
    im_id id;

    im_loops loops;
    bool waiting;

    tape_packed tape;
};

static_assert(sizeof(struct im_printer) == 12);

void im_printer_config(struct im_config *);
