/* transmit.h
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

struct im_config;


// -----------------------------------------------------------------------------
// transmit
// -----------------------------------------------------------------------------

struct legion_packed im_transmit
{
    im_id id;
    uint8_t channel;
    legion_pad(5);
    struct coord target;
    struct im_packet packet;
};

static_assert(sizeof(struct im_transmit) == 48);

void im_transmit_config(struct im_config *);
