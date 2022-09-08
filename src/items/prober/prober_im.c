/* prober_im.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "items/io.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// prober
// -----------------------------------------------------------------------------

static const word im_prober_empty = -1;
static const uint64_t im_prober_div = 1000;

static void im_prober_init(void *state, struct chunk *chunk, id id)
{
    struct im_prober *prober = state;
    (void) chunk;

    prober->id = id;
    prober->result = im_prober_empty;
}

static void im_prober_reset(struct im_prober *prober)
{
    prober->item = ITEM_NIL;
    prober->coord = coord_nil();
    prober->result = im_prober_empty;
    prober->work.left = 0;
    prober->work.cap = 0;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_prober_step(void *state, struct chunk *chunk)
{
    struct im_prober *prober = state;

    if (!prober->item) return;
    if (prober->result != im_prober_empty) return;
    if (prober->work.left) { prober->work.left--; return; }

    ssize_t ret = world_scan(chunk_world(chunk), prober->coord, prober->item);
    prober->result = ret < 0 ? 0 : ret;
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_prober_io_state(
        struct im_prober *prober, struct chunk *chunk, id src,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, prober->id, IO_STATE, len, 1)) return;
    word value = 0;

    switch (args[0])
    {
    case IO_TARGET: { value = coord_to_u64(prober->coord);  break; }
    case IO_ITEM: { value = prober->item; break; }
    default: { chunk_log(chunk, prober->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, prober->id, src, &value, 1);
}

static void im_prober_io_probe(
        struct im_prober *prober, struct chunk *chunk,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, prober->id, IO_PROBE, len, 1)) return;

    enum item item = args[0];
    if (!item_validate(item))
        return chunk_log(chunk, prober->id, IO_SCAN, IOE_A0_INVALID);

    struct coord coord = coord_nil();
    if (len >= 2) coord = coord_from_u64(args[1]);

    struct coord origin = chunk_star(chunk)->coord;
    if (coord_is_nil(coord)) coord = origin;

    prober->item = item;
    prober->coord = coord;

    uint64_t delta = coord_dist(origin, coord) / im_prober_div;
    if (delta >= UINT8_MAX) {
        im_prober_reset(prober);
        return chunk_log(chunk, prober->id, IO_SCAN, IOE_OUT_OF_RANGE);
    }

    prober->work.cap = prober->work.left = delta;
    prober->result = im_prober_empty;
}

static void im_prober_io_value(
        struct im_prober *prober, struct chunk *chunk, id src)
{
    chunk_io(chunk, IO_RETURN, prober->id, src, &prober->result, 1);

    if (prober->result != im_prober_empty)
        im_prober_reset(prober);
}

static void im_prober_io(
        void *state, struct chunk *chunk,
        enum io io, id src,
        const word *args, size_t len)
{
    struct im_prober *prober = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, prober->id, src, NULL, 0); return; }
    case IO_STATE: { im_prober_io_state(prober, chunk, src, args, len); return; }

    case IO_PROBE: { im_prober_io_probe(prober, chunk, args, len); return; }
    case IO_VALUE: { im_prober_io_value(prober, chunk, src); return; }
    case IO_RESET: { im_prober_reset(prober); return; }

    default: { return; }
    }
}

static const word im_prober_io_list[] =
{
    IO_PING,
    IO_STATE,

    IO_PROBE,
    IO_VALUE,
    IO_RESET,
};
