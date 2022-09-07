/* port.h
   Rémi Attab (remi.attab@gmail.com), 25 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once


#include "common.h"
#include "game/id.h"
#include "items/item.h"

struct im_config;

// -----------------------------------------------------------------------------
// port
// -----------------------------------------------------------------------------

enum { im_port_speed = 100 };

struct legion_packed im_port
{
    id_t id;

    legion_pad(2);

    struct legion_packed { enum item item; uint8_t count; } has, want;
    struct coord target;
};

static_assert(sizeof(struct im_port) == 16);


inline word_t im_port_pack(enum item item, uint8_t count)
{
    return (((uint64_t) count) << 8) | item;
}

inline void im_port_unpack(word_t word, enum item *item, uint8_t *count)
{
    *item = word & 0xFF;
    *count = word >> 8;
}


void im_port_config(struct im_config *);
