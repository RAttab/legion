/* receive_im.c
   Rémi Attab (remi.attab@gmail.com), 26 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/io.h"
#include "items/item.h"
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

static void im_receive_init(void *state, struct chunk *chunk, id_t id)
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

static void im_receive_io_status(
        struct im_receive *receive, struct chunk *chunk, id_t src)
{
    word_t value = coord_to_u64(receive->target);
    chunk_io(chunk, IO_STATE, receive->id, src, &value, 1);
}

static void im_receive_io_channel(
        struct im_receive *receive, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (len < 1) return;
    if (args[0] >= im_channel_max) return;

    im_receive_unlisten(receive, chunk);
    receive->channel = args[0];
    im_receive_listen(receive, chunk);
}

static void im_receive_io_target(
        struct im_receive *receive, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (len < 1) return;

    im_receive_unlisten(receive, chunk);
    receive->target = coord_from_u64(args[0]);
    im_receive_listen(receive, chunk);
}

static void im_receive_io_receive(
        struct im_receive *receive, struct chunk *chunk, id_t src)
{
    size_t len = 0;
    word_t data[im_packet_max] = {0};

    if (receive->tail < receive->head) {
        const size_t tail = receive->tail % im_receive_cap(receive);
        const struct im_packet *packet = receive->buffer + tail;

        len = packet->len;
        memcpy(data, packet->data, packet->len * sizeof(packet->data[0]));

        receive->tail++;
    }

    chunk_io(chunk, IO_RECV, receive->id, src, data, len);
}

static void im_receive_io_recv(
        struct im_receive *receive,
        const word_t *args, size_t len)
{
    assert(len >= 1);

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
    assert(packet->len < len);
    memcpy(packet->data, args + 1, packet->len * sizeof(packet->data[0]));

    receive->head++;
}

static void im_receive_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_receive *receive = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, receive->id, src, NULL, 0); return; }
    case IO_STATUS: { im_receive_io_status(receive, chunk, src); return; }
    case IO_RESET: { im_receive_reset(receive, chunk); return; }

    case IO_CHANNEL: { im_receive_io_channel(receive, chunk, args, len); return; }
    case IO_TARGET: { im_receive_io_target(receive, chunk, args, len); return; }
    case IO_RECEIVE: { im_receive_io_receive(receive, chunk, src); return; }

    case IO_RECV: { im_receive_io_recv(receive, args, len); return; }

    default: { return; }
    }
}

static const word_t im_receive_io_list[] =
{
    IO_PING,
    IO_STATUS,
    IO_RESET,

    IO_CHANNEL,
    IO_TARGET,
    IO_RECEIVE,
};
