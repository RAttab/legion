/* packer.h
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct im_config;


// -----------------------------------------------------------------------------
// packer
// -----------------------------------------------------------------------------

struct legion_packed im_packer
{
    im_id id;

    enum item item;
    im_loops loops;
    bool waiting;

    legion_pad(1);
};

static_assert(sizeof(struct im_packer) == 6);

void im_packer_config(struct im_config *);
