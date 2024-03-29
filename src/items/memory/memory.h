/* memory.h
   Rémi Attab (remi.attab@gmail.com), 20 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct im_config;


// -----------------------------------------------------------------------------
// memory
// -----------------------------------------------------------------------------

struct legion_packed im_memory
{
    im_id id;
    uint8_t len;
    legion_pad(5);
    vm_word data[];
};

static_assert(sizeof(struct im_memory) == 8);

void im_memory_config(struct im_config *);
