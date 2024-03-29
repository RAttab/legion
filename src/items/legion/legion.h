/* legion.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct im_config;


// -----------------------------------------------------------------------------
// legion
// -----------------------------------------------------------------------------

struct legion_packed im_legion
{
    im_id id;

    legion_pad(2);

    mod_id mod;
};

static_assert(sizeof(struct im_legion) == 8);

const enum item *im_legion_cargo(enum item type);

void im_legion_config(struct im_config *);
