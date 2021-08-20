/* research.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "items/item.h"
#include "utils/rng.h"

struct im_config;


// -----------------------------------------------------------------------------
// research
// -----------------------------------------------------------------------------

enum legion_packed im_research_state
{
    im_research_idle = 0,
    im_research_waiting = 1,
    im_research_working = 2,
};

struct legion_packed im_research
{
    id_t id;

    enum item item;
    enum im_research_state state;
    struct { uint8_t left; uint8_t cap; } work;

    struct rng rng;
};

static_assert(sizeof(struct im_research) == 16);

void im_research_config(struct im_config *);
