/* fusion.h
   Rémi Attab (remi.attab@gmail.com), 15 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "items/item.h"
#include "items/types.h"

struct im_config;


// -----------------------------------------------------------------------------
// fusion
// -----------------------------------------------------------------------------

static const enum item im_fusion_input_item = ITEM_ROD;

static const im_energy im_fusion_energy_output = 20;
static const im_energy im_fusion_energy_rod = 1024;
static const im_energy im_fusion_energy_cap = im_fusion_energy_rod * 16;

struct legion_packed im_fusion
{
    im_id id;

    bool paused;
    bool waiting;
    im_energy energy;
};

static_assert(sizeof(struct im_fusion) == 8);

void im_fusion_config(struct im_config *);
