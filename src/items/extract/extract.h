/* extract.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct im_config;


// -----------------------------------------------------------------------------
// extract
// -----------------------------------------------------------------------------

struct legion_packed im_extract
{
    im_id id;

    im_loops loops;
    bool waiting;

    tape_packed tape;
};

static_assert(sizeof(struct im_extract) == 12);

void im_extract_config(struct im_config *);
