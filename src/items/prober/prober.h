/* prober.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct im_config;


// -----------------------------------------------------------------------------
// scan
// -----------------------------------------------------------------------------

struct legion_packed im_prober
{
    im_id id;

    struct { im_work left, cap; } work;
    enum item item;

    legion_pad(3);

    struct coord coord;
    vm_word result;

};

static_assert(sizeof(struct im_prober) == 24);

void im_prober_config(struct im_config *);

im_work im_prober_work_cap(struct coord origin, struct coord target);
