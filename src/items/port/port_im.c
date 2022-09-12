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

static void im_port_init(void *state, struct chunk *chunk, im_id id)
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
        vm_word data = 0;
        if (!chunk_lanes_dock(chunk, &data)) return;

        im_port_unpack(data, &port->has.item, &port->has.count);
        if (!port->has.item) port->has.item = port->want.item;
    }

    if (port->has.item != port->want.item) {
        if (!chunk_ports_produce(chunk, port->id, port->has.item)) return;
        port->has.count--;

        if (port->has.count) return;
        port->has.item = port->want.item;
    }

    if (port->has.count < port->want.count) {
        enum item item = chunk_ports_consume(chunk, port->id);
        if (!item) { chunk_ports_request(chunk, port->id, port->want.item); return; }
        assert(item == port->want.item);
        port->has.count++;
        return;
    }

    if (!coord_is_nil(port->target)) {
        const vm_word data = im_port_pack(port->has.item, port->has.count);
        chunk_lanes_launch(chunk, ITEM_PILL, im_port_speed, port->target, &data, 1);

        port->has.item = 0;
        port->has.count = 0;
        return;
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_port_io_state(
        struct im_port *port, struct chunk *chunk, im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, port->id, IO_STATE, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case IO_TARGET: { value = coord_to_u64(port->target); break; }
    case IO_ITEM: { value = port->want.item; break; }
    case IO_LOOP: { value = port->want.count; break; }
    case IO_HAS_ITEM: { value = port->has.item; break; }
    case IO_HAS_LOOP: { value = port->has.count; break; }
    default: { chunk_log(chunk, port->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, port->id, src, &value, 1);
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
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, port->id, IO_ITEM, len, 1)) return;

    enum item item = args[0];
    if (!item_validate(args[0]))
        return chunk_log(chunk, port->id, IO_ITEM, IOE_A0_INVALID);

    if (!im_check_known(chunk, port->id, IO_ITEM, item)) return;

    if (item == port->want.item) return;
    port->want.item = item;
    port->want.count = item ? 1 : 0;

    if (len >= 2) {
        if (args[1] < 0 || args[1] > UINT8_MAX)
            return chunk_log(chunk, port->id, IO_ITEM, IOE_A1_INVALID);
        port->want.count = args[1];
    }

    chunk_ports_reset(chunk, port->id);
}

static void im_port_io_target(
        struct im_port *port, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, port->id, IO_TARGET, len, 1)) return;

    struct coord coord = coord_from_u64(args[0]);
    if (!coord_validate(args[0]))
        return chunk_log(chunk, port->id, IO_TARGET, IOE_A0_INVALID);

    port->target = coord;
}

static void im_port_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_port *port = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, port->id, src, NULL, 0); return; }
    case IO_STATE: { im_port_io_state(port, chunk, src, args, len); return; }

    case IO_RESET: { im_port_io_reset(port, chunk); return; }
    case IO_ITEM: { im_port_io_item(port, chunk, args, len); return; }

    case IO_TARGET: {im_port_io_target(port, chunk, args, len); return; }

    default: { return; }
    }
}

static const vm_word im_port_io_list[] =
{
    IO_PING,
    IO_STATE,

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
    if (!port->want.item && !port->has.item) return false;

    const struct tape_info *info = tapes_info(port->has.item);

    *flow = (struct flow) {
        .id = port->id,
        .loops = port->want.count,
        .target = port->want.item,
        .item = port->has.item,
        .rank = info ? info->rank : 1,
    };

    if (port->want.item == port->has.item) {
        flow->state = tape_input;
        flow->tape_it = port->has.count;
        flow->tape_len = port->want.count;
    }
    else {
        flow->state = tape_output;
        flow->tape_len = port->has.count;
    }

    return true;
}
