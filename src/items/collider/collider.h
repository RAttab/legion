/* collider.h
   Rémi Attab (remi.attab@gmail.com), 27 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/id.h"
#include "game/tape.h"
#include "items/types.h"

struct im_config;


// -----------------------------------------------------------------------------
// collider
// -----------------------------------------------------------------------------

enum
{
    im_collider_size_max = 64,
};

static const enum item im_collider_grow_item = ITEM_ACCELERATOR;
static const enum item im_collider_junk = ITEM_ELEM_O;

enum legion_packed im_collider_op
{
    im_collider_nil = 0,
    im_collider_grow,
    im_collider_in,
    im_collider_work,
    im_collider_out,
};


struct legion_packed im_collider
{
    id_t id;

    uint8_t size;
    uint8_t rate;

    enum im_collider_op op;
    bool waiting;
    loops_t loops;

    struct { uint8_t left, cap; } work;
    struct { enum item item; uint8_t it, len; } out;

    legion_pad(1);

    struct rng rng;
    tape_packed_t tape;
};

static_assert(sizeof(struct im_collider) == 32);


void im_collider_config(struct im_config *);
uint8_t im_collider_rate(uint8_t size);
