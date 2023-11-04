/* collider.h
   Rémi Attab (remi.attab@gmail.com), 27 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct im_config;


// -----------------------------------------------------------------------------
// collider
// -----------------------------------------------------------------------------

enum im_collider_op : uint8_t
{
    im_collider_nil = 0,
    im_collider_grow,
    im_collider_tape,
};


struct legion_packed im_collider
{
    im_id id;

    uint8_t size;
    uint8_t rate;

    enum im_collider_op op;
    bool waiting;
    im_loops loops;

    struct { enum item item; uint8_t it, len; } out;

    legion_pad(6);

    struct rng rng;
    tape_packed tape;
};

static_assert(sizeof(struct im_collider) == 32);


void im_collider_config(struct im_config *);
uint8_t im_collider_rate(uint8_t size);
