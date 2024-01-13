/* brain.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct im_config;


// -----------------------------------------------------------------------------
// brain
// -----------------------------------------------------------------------------

struct legion_packed im_brain
{
    im_id id;

    bool fault;
    bool debug;
    vm_ip breakpoint;

    mod_id mod_id;
    legion_pad(12);
    const struct mod *mod;

    struct im_packet msg;

    struct vm vm;
};

static_assert(sizeof(struct im_brain) == sys_cache_line_len + sizeof(struct vm));

void im_brain_config(struct im_config *);
