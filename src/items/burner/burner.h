/* burner.h
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "items/item.h"
#include "items/types.h"

struct im_config;


// -----------------------------------------------------------------------------
// burner
// -----------------------------------------------------------------------------

enum legion_packed im_burner_op
{
    im_burner_nil = 0,
    im_burner_in,
    im_burner_work,
};

struct legion_packed im_burner
{
    id_t id;

    enum im_burner_op op;
    enum item item;
    uint8_t output;

    bool waiting;
    loops_t loops;
    struct { work_t left, cap; } work;
};

static_assert(sizeof(struct im_burner) == 12);

void im_burner_config(struct im_config *);