/* scanner.h
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

enum legion_packed im_scanner_state
{
    im_scanner_idle = 0,
    im_scanner_wide = 1,
    im_scanner_target = 2,
};

struct legion_packed im_scanner
{
    id_t id;

    enum im_scanner_state state;
    struct { uint8_t left; uint8_t cap; } work;
    legion_pad(1);

    union
    {
        struct world_scan_it wide;
        struct { enum item item; struct coord coord; } target;
    } type;

    word_t result;
};

static_assert(sizeof(struct im_scanner) == 32);

void im_scanner_config(struct im_config *);
