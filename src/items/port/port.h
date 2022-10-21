/* port.h
   Rémi Attab (remi.attab@gmail.com), 25 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


#include "common.h"
#include "game/id.h"
#include "db/items.h"

struct im_config;

// -----------------------------------------------------------------------------
// port
// -----------------------------------------------------------------------------

enum { im_port_speed = 100 };

enum legion_packed im_port_state
{
    im_port_idle = 0,
    im_port_docking,
    im_port_docked,
    im_port_loading,
    im_port_unloading,
};

struct legion_packed im_port
{
    im_id id;

    struct cargo has, want;

    enum im_port_state state;

    struct legion_packed {
        enum item item;
        struct coord coord;
    } input;

    struct coord origin, target;
};

static_assert(sizeof(struct im_port) == 32);

void im_port_config(struct im_config *);
