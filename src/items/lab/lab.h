/* lab.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct im_config;


// -----------------------------------------------------------------------------
// lab
// -----------------------------------------------------------------------------

enum im_lab_state : uint8_t
{
    im_lab_idle = 0,
    im_lab_waiting = 1,
    im_lab_working = 2,
};

struct legion_packed im_lab
{
    im_id id;

    enum item item;
    enum im_lab_state state;
    struct { im_work left, cap; } work;

    legion_pad(2);

    struct rng rng;
};

static_assert(sizeof(struct im_lab) == 16);

void im_lab_config(struct im_config *);
