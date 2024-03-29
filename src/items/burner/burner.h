/* burner.h
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct im_config;


// -----------------------------------------------------------------------------
// burner
// -----------------------------------------------------------------------------

enum im_burner_op : uint8_t
{
    im_burner_nil = 0,
    im_burner_in,
    im_burner_work,
};

struct legion_packed im_burner
{
    im_id id;

    enum im_burner_op op;
    enum item item;
    uint8_t output;

    bool waiting;
    im_loops loops;
    struct { im_work left, cap; } work;

    legion_pad(3);
};

static_assert(sizeof(struct im_burner) == 12);

void im_burner_config(struct im_config *);

im_energy im_burner_energy(enum item);
im_work im_burner_work_cap(enum item);
