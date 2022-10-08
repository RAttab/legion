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

static void im_port_step_dock(struct im_port *port, struct chunk *chunk)
{
    struct pills_ret ret = chunk_pills_dock(
            chunk, port->input.coord, port->input.item);
    if (!ret.ok) return;

    port->state = im_port_docked;
    port->origin = ret.coord;
    port->has = ret.cargo;

    if (!port->has.count)
        port->has.item = port->want.item;
}

static void im_port_step_unload(struct im_port *port, struct chunk *chunk)
{
    if (port->state == im_port_docked) {
        chunk_ports_produce(chunk, port->id, port->has.item);
        port->state = im_port_unloading;
        return;
    }

    if (!chunk_ports_consumed(chunk, port->id)) return;
    port->state = im_port_docked;
    port->has.count--;

    if (!port->has.count)
        port->has.item = port->want.item;
}

static void im_port_step_load(struct im_port *port, struct chunk *chunk)
{
    if (port->state == im_port_docked) {
        chunk_ports_request(chunk, port->id, port->want.item);
        port->state = im_port_loading;
        return;
    }

    if (!chunk_ports_consume(chunk, port->id)) return;
    port->state = im_port_docked;
    port->has.count++;
}

static void im_port_step_launch(struct im_port *port, struct chunk *chunk)
{
    const vm_word data = cargo_to_word(port->has);
    struct coord dst = coord_is_nil(port->target) ? port->origin : port->target;

    chunk_lanes_launch(chunk, ITEM_PILL, im_port_speed, dst, &data, 1);

    port->state = im_port_docking;
    port->has = (struct cargo) {0};
    port->origin = coord_nil();
}

static void im_port_step(void *state, struct chunk *chunk)
{
    struct im_port *port = state;

    switch (port->state)
    {
    case im_port_idle: { return; }

    case im_port_docking: { im_port_step_dock(port, chunk); return; }
    case im_port_loading: { im_port_step_load(port, chunk); return; }
    case im_port_unloading: { im_port_step_unload(port, chunk); return; }

    case im_port_docked: {

        if (port->has.item != port->want.item)
            im_port_step_unload(port, chunk);

        else if (port->has.count < port->want.count)
            im_port_step_load(port, chunk);

        else im_port_step_launch(port, chunk);

        return;
    }

    default: { assert(false); }
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
    if (port->state >= im_port_docked) {
        chunk_ports_reset(chunk, port->id);
        if (!chunk_pills_undock(chunk, port->origin, port->has))
            chunk_log(chunk, port->id, IO_RESET, IOE_OUT_OF_SPACE);
    }

    legion_zero_from(port, has);
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

    uint8_t count = 0;
    if (len >= 2) {
        if (args[1] < 0 || args[1] > UINT8_MAX)
            return chunk_log(chunk, port->id, IO_ITEM, IOE_A1_INVALID);
        count = args[1];
    }

    port->want.item = item;
    port->want.count = !count && item ? 1 : count;
    chunk_ports_reset(chunk, port->id);
}

static void im_port_io_target(
        struct im_port *port, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, port->id, IO_TARGET, len, 1)) return;

    // nil -> return to sender
    port->target = coord_from_u64(args[0]);
}

static void im_port_io_input(
        struct im_port *port, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, port->id, IO_INPUT, len, 1)) return;

    // nil -> input all
    enum item item = args[0];
    if (args[0] < 0 || args[0] > ITEM_MAX)
        return chunk_log(chunk, port->id, IO_INPUT, IOE_A0_INVALID);

    struct coord coord = coord_nil();
    if (len >= 2) coord_from_u64(args[1]);

    port->input.item = item;
    port->input.coord = coord;
}

static void im_port_io_activate(struct im_port *port)
{
    if (port->state != im_port_idle) return;
    port->state = im_port_docking;
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
    case IO_INPUT: {im_port_io_input(port, chunk, args, len); return; }
    case IO_ACTIVATE: {im_port_io_activate(port); return; }

    default: { return; }
    }
}

static const struct io_cmd im_port_io_list[] =
{
    { IO_PING,   0, {} },
    { IO_STATE,  1, { { "state", true } }},
    { IO_RESET,  0, {} },

    { IO_ITEM,   2, { { "item", true },
                      { "count", false } }},
    { IO_TARGET, 1, { { "coord", true } }},
    { IO_INPUT,  2, { { "item", true },
                      { "coord", false } }},

    { IO_ACTIVATE, 0, {} },
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_port_flow(const void *state, struct flow *flow)
{
    const struct im_port *port = state;
    if (port->state == im_port_idle) return false;

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
        flow->tape_len = port->has.count;
    }
    else {
        flow->state = tape_output;
        flow->tape_len = port->has.count;
    }

    return true;
}
