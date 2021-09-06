/* port_im.c
   Rémi Attab (remi.attab@gmail.com), 25 Aug 2021
   FreeBSD-style copyright and disclaimer apply
*/


#include "common.h"
#include "items/io.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// port
// -----------------------------------------------------------------------------

static void im_port_init(void *state, struct chunk *chunk, id_t id)
{
    (void) chunk;

    struct im_port *port = state;
    port->id = id;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_port_step(void *state, struct chunk *chunk)
{
    struct im_port *port = state;
    if (coord_is_nil(port->target)) return;

    if (!port->has.item) {
        word_t data = 0;
        if (!chunk_lanes_dock(chunk, &data)) return;

        im_port_unpack(data, &port->has.item, &port->has.count);
        if (!port->has.item) port->has.item = port->want.item;
    }

    if (port->has.item != port->want.item) {
        if (!chunk_ports_produce(chunk, port->id, port->has.item)) return;
        port->has.count--;

        if (!port->has.count) port->has.item = port->want.item;
        return;
    }

    if (port->has.count < port->want.count) {
        enum item item = chunk_ports_consume(chunk, port->id);
        if (!item) { chunk_ports_request(chunk, port->id, port->want.item); return; }
        assert(item == port->want.item);
        port->has.count++;
        return;
    }

    if (!coord_is_nil(port->target)) {
        const word_t data = im_port_pack(port->has.item, port->has.count);
        chunk_lanes_launch(chunk, ITEM_BULLET, im_port_speed, port->target, &data, 1);

        port->has.item = 0;
        port->has.count = 0;
        return;
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_port_io_status(
        struct im_port *port, struct chunk *chunk, id_t src)
{
    word_t value[] = {
        vm_pack(port->want.count, port->want.item),
        coord_to_u64(port->target),
    };
    chunk_io(chunk, IO_STATE, port->id, src, value, array_len(value));
}

static void im_port_io_reset(struct im_port *port, struct chunk *chunk)
{
    chunk_ports_reset(chunk, port->id);
    port->want.item = ITEM_NIL;
    port->want.count = 0;
    port->has.item = ITEM_NIL;
    port->has.count = 0;
    port->target = coord_nil();
}

static void im_port_io_item(
        struct im_port *port, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (len < 1) return;

    enum item item = args[0];
    if (args[0] <= 0 || args[0] >= ITEM_MAX) return;
    if (!world_lab_known(chunk_world(chunk), item)) return;
    if (item == port->want.item) return;
    port->want.item = item;

    port->want.count = 1;
    if (!item) port->want.count = 0;
    else if (len >= 2 && args[1] > 0)
        port->want.count = legion_min(args[1], UINT8_MAX);

    chunk_ports_reset(chunk, port->id);
}

static void im_port_io_target(
        struct im_port *port, const word_t *args, size_t len)
{
    if (len < 1) return;
    port->target = coord_from_u64(args[0]);
}

static void im_port_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_port *port = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, port->id, src, NULL, 0); return; }
    case IO_STATUS: { im_port_io_status(port, chunk, src); return; }

    case IO_RESET: { im_port_io_reset(port, chunk); return; }
    case IO_ITEM: { im_port_io_item(port, chunk, args, len); return; }

    case IO_TARGET: {im_port_io_target(port, args, len); return; }

    default: { return; }
    }
}

static const word_t im_port_io_list[] =
{
    IO_PING,
    IO_STATUS,

    IO_RESET,
    IO_ITEM,

    IO_TARGET,
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_port_flow(const void *state, struct flow *flow)
{
    const struct im_port *port = state;
    if (coord_is_nil(port->target)) return false;

    const struct tape_info *info = tapes_info(port->has.item);

    *flow = (struct flow) {
        .id = port->id,
        .loops = port->want.count,
        .target = port->want.item,
        .rank = info ? info->rank : 1,
    };

    if (port->want.item == port->has.item)
        flow->in = port->has.item;
    else flow->out = port->has.item;

    return true;
}
