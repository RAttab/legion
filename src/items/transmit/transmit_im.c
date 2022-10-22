/* transmit_im.c
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "db/io.h"
#include "db/items.h"
#include "items/types.h"
#include "items/receive/receive.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// transmit
// -----------------------------------------------------------------------------

static void im_transmit_init(void *state, struct chunk *chunk, im_id id)
{
    struct im_transmit *transmit = state;
    (void) chunk;

    transmit->id = id;
}


static void im_transmit_reset(struct im_transmit *transmit, struct chunk *chunk)
{
    (void) chunk;

    transmit->target = coord_nil();
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_transmit_io_state(
        struct im_transmit *transmit, struct chunk *chunk, im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, transmit->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case io_target: { value = coord_to_u64(transmit->target); break; }
    case io_channel: { value = transmit->channel; break; }
    default: { chunk_log(chunk, transmit->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, transmit->id, src, &value, 1);
}

static void im_transmit_io_channel(
        struct im_transmit *transmit, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, transmit->id, io_channel, len, 1)) return;

    uint8_t channel = args[0];
    if (args[0] < 0 || args[0] >= im_channels_max)
        return chunk_log(chunk, transmit->id, io_channel, ioe_a0_invalid);

    transmit->channel = channel;
}

static void im_transmit_io_target(
        struct im_transmit *transmit, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, transmit->id, io_target, len, 1)) return;

    struct coord target = coord_from_u64(args[0]);
    if (!coord_validate(args[0]))
        return chunk_log(chunk, transmit->id, io_target, ioe_a0_invalid);

    transmit->target = target;
}

static void im_transmit_io_transmit(
        struct im_transmit *transmit, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, transmit->id, io_transmit, len, 1)) return;

    if (coord_is_nil(transmit->target))
        return chunk_log(chunk, transmit->id, io_transmit, ioe_invalid_state);

    const size_t packet_len = legion_min(len, (size_t) im_packet_max);
    const size_t packet_speed = im_transmit_launch_speed;

    vm_word packet[1 + packet_len];
    packet[0] = im_packet_pack(transmit->channel, packet_len);
    memcpy(packet+1, args, packet_len * sizeof(packet[0]));

    chunk_lanes_launch(chunk,
            item_data, packet_speed, transmit->target, packet, 1+packet_len);
}

static void im_transmit_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_transmit *transmit = state;

    switch(io)
    {
    case io_ping: { chunk_io(chunk, io_pong, transmit->id, src, NULL, 0); return; }
    case io_state: { im_transmit_io_state(transmit, chunk, src, args, len); return; }
    case io_reset: { im_transmit_reset(transmit, chunk); return; }

    case io_channel: { im_transmit_io_channel(transmit, chunk, args, len); return; }
    case io_target: { im_transmit_io_target(transmit, chunk, args, len); return; }
    case io_transmit: { im_transmit_io_transmit(transmit, chunk, args, len); return; }

    default: { return; }
    }
}

static const struct io_cmd im_transmit_io_list[] =
{
    { io_ping,     0, {} },
    { io_state,    1, { { "state", true } }},
    { io_reset,    0, {} },

    { io_channel,  1, { { "channel", true } }},
    { io_target,   1, { { "coord", true } }},
    { io_transmit, 3, { { "msg[0]", true },
                        { "msg[1]", false },
                        { "msg[2]", false } }},
};
