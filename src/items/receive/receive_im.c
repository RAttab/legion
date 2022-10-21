/* receive_im.c
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "db/io.h"
#include "db/items.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// receive
// -----------------------------------------------------------------------------

size_t im_receive_cap(const struct im_receive *receive)
{
    (void) receive;
    return 1;
}

static uint8_t im_receive_len(struct im_receive *receive)
{
    assert(im_receive_cap(receive) == 1);
    return receive->head != receive->tail ? 1 : 0;
}

static void im_receive_init(void *state, struct chunk *chunk, im_id id)
{
    struct im_receive *receive = state;
    (void) chunk;

    receive->id = id;
}


static void im_receive_listen(struct im_receive *receive, struct chunk *chunk)
{
    if (coord_is_nil(receive->target)) return;
    chunk_lanes_listen(chunk, receive->id, receive->target, receive->channel);
}

static void im_receive_unlisten(struct im_receive *receive, struct chunk *chunk)
{
    if (coord_is_nil(receive->target)) return;
    chunk_lanes_unlisten(chunk, receive->id, receive->target, receive->channel);
}

static void im_receive_reset(struct im_receive *receive, struct chunk *chunk)
{
    im_receive_unlisten(receive, chunk);
    receive->channel = 0;
    receive->head = receive->tail = 0;
    receive->target = coord_nil();
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_receive_io_state(
        struct im_receive *receive, struct chunk *chunk, im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, receive->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case io_target: { value = coord_to_u64(receive->target); break; }
    case io_channel: { value = receive->channel; break; }
    case io_loop: { value = im_receive_len(receive); break; }
    default: { chunk_log(chunk, receive->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, receive->id, src, &value, 1);
}

static void im_receive_io_channel(
        struct im_receive *receive, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, receive->id, io_channel, len, 1)) return;

    uint8_t channel = args[0];
    if (args[0] < 0 || args[0] >= im_channels_max)
        return chunk_log(chunk, receive->id, io_channel, ioe_a0_invalid);

    im_receive_unlisten(receive, chunk);
    receive->channel = channel;
    im_receive_listen(receive, chunk);
}

static void im_receive_io_target(
        struct im_receive *receive, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, receive->id, io_target, len, 1)) return;

    struct coord target = coord_from_u64(args[0]);
    if (!coord_validate(args[0]))
        return chunk_log(chunk, receive->id, io_target, ioe_a0_invalid);

    im_receive_unlisten(receive, chunk);
    receive->target = target;
    im_receive_listen(receive, chunk);
}

static void im_receive_io_receive(
        struct im_receive *receive, struct chunk *chunk, im_id src)
{
    size_t len = 0;
    vm_word data[im_packet_max] = {0};

    if (receive->tail < receive->head) {
        const size_t tail = receive->tail % im_receive_cap(receive);
        const struct im_packet *packet = receive->buffer + tail;

        len = packet->len;
        memcpy(data, packet->data, packet->len * sizeof(packet->data[0]));

        receive->tail++;
    }

    chunk_io(chunk, io_recv, receive->id, src, data, len);
}

static void im_receive_io_recv(
        struct im_receive *receive, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, receive->id, io_recv, len, 1)) return;

    const size_t cap = im_receive_cap(receive);
    if (receive->head == UINT8_MAX) {
        receive->head %= cap;
        receive->tail %= cap;
    }

    size_t head = receive->head % cap;
    size_t tail = receive->tail % cap;
    if (tail == head && receive->tail < receive->head) return;

    struct im_packet *packet = receive->buffer + head;
    im_packet_unpack(args[0], &packet->chan, &packet->len);

    if (packet->len >= len || packet->chan >= im_channels_max)
        return chunk_log(chunk, receive->id, io_recv, ioe_a0_invalid);

    memcpy(packet->data, args + 1, packet->len * sizeof(packet->data[0]));
    receive->head++;
}

static void im_receive_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_receive *receive = state;

    switch(io)
    {
    case io_ping: { chunk_io(chunk, io_pong, receive->id, src, NULL, 0); return; }
    case io_state: { im_receive_io_state(receive, chunk, src, args, len); return; }
    case io_reset: { im_receive_reset(receive, chunk); return; }

    case io_channel: { im_receive_io_channel(receive, chunk, args, len); return; }
    case io_target: { im_receive_io_target(receive, chunk, args, len); return; }
    case io_receive: { im_receive_io_receive(receive, chunk, src); return; }

    case io_recv: { im_receive_io_recv(receive, chunk, args, len); return; }

    default: { return; }
    }
}

static const struct io_cmd im_receive_io_list[] =
{
    { io_ping,    0, {} },
    { io_state,   1, { { "state", true } }},
    { io_reset,   0, {} },

    { io_channel, 1, { { "channel", true } }},
    { io_target,  1, { { "coord", true } }},
    { io_receive, 0, {} },
};
