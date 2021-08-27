/* transmit_im.c
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/io.h"
#include "items/item.h"
#include "items/types.h"
#include "items/receive/receive.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// transmit
// -----------------------------------------------------------------------------

static void im_transmit_init(void *state, struct chunk *chunk, id_t id)
{
    (void) chunk;

    struct im_transmit *transmit = state;
    transmit->id = id;
}


static void im_transmit_reset(struct im_transmit *transmit, struct chunk *)
{
    transmit->target = coord_nil();
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_transmit_io_status(
        struct im_transmit *transmit, struct chunk *chunk, id_t src)
{
    word_t value = coord_to_u64(transmit->target);
    chunk_io(chunk, IO_STATE, transmit->id, src, &value, 1);
}

static void im_transmit_io_channel(
        struct im_transmit *transmit, const word_t *args, size_t len)
{
    if (len < 1) return;
    if (args[0] >= im_channel_max) return;

    transmit->channel = args[0];
}

static void im_transmit_io_target(
        struct im_transmit *transmit, const word_t *args, size_t len)
{
    if (len < 1) return;
    transmit->target = coord_from_u64(args[0]);
}

static void im_transmit_io_transmit(
        struct im_transmit *transmit, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (len < 1) return;
    if (coord_is_nil(transmit->target)) return;

    const size_t packet_len = legion_min(len, (size_t) im_packet_max);
    const size_t packet_speed = im_transmit_speed;

    word_t packet[1 + packet_len];
    packet[0] = im_packet_pack(transmit->channel, packet_len);
    memcpy(packet+1, args, packet_len * sizeof(packet[0]));

    chunk_lanes_launch(
            chunk, ITEM_DATA, packet_speed, transmit->target, packet, packet_len);
}

static void im_transmit_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_transmit *transmit = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, transmit->id, src, NULL, 0); return; }
    case IO_STATUS: { im_transmit_io_status(transmit, chunk, src); return; }
    case IO_RESET: { im_transmit_reset(transmit, chunk); return; }

    case IO_CHANNEL: { im_transmit_io_channel(transmit, args, len); return; }
    case IO_TARGET: { im_transmit_io_target(transmit, args, len); return; }
    case IO_TRANSMIT: { im_transmit_io_transmit(transmit, chunk, args, len); return; }

    default: { return; }
    }
}

static const word_t im_transmit_io_list[] =
{
    IO_PING,
    IO_STATUS,
    IO_RESET,

    IO_CHANNEL,
    IO_TARGET,
    IO_TRANSMIT,
};