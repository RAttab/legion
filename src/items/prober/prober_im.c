/* prober_im.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "db/io.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// prober
// -----------------------------------------------------------------------------

static const vm_word im_prober_empty = -1;
static const uint64_t im_prober_div = 1000;

static void im_prober_init(void *state, struct chunk *chunk, im_id id)
{
    struct im_prober *prober = state;
    (void) chunk;

    prober->id = id;
    prober->result = im_prober_empty;
}

static void im_prober_reset(struct im_prober *prober)
{
    prober->item = item_nil;
    prober->coord = coord_nil();
    prober->result = im_prober_empty;
    prober->work.left = 0;
    prober->work.cap = 0;
}

im_work im_prober_work_cap(struct coord origin, struct coord target) {
    uint64_t delta = coord_dist(origin, target) / im_prober_div;
    return delta > UINT8_MAX ? UINT8_MAX : delta;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_prober_step(void *state, struct chunk *chunk)
{
    struct im_prober *prober = state;

    if (!prober->item) return;
    if (prober->result != im_prober_empty) return;

    if (prober->work.left) {
        if (energy_consume(chunk_energy(chunk), im_prober_work_energy))
            prober->work.left--;
        return;
    }

    ssize_t ret = world_scan(chunk_world(chunk), prober->coord, prober->item);
    prober->result = ret < 0 ? 0 : ret;
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_prober_io_state(
        struct im_prober *prober, struct chunk *chunk, im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, prober->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0])
    {
    case io_target: { value = coord_to_u64(prober->coord);  break; }
    case io_item: { value = prober->item; break; }
    default: { chunk_log(chunk, prober->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, prober->id, src, &value, 1);
}

static void im_prober_io_probe(
        struct im_prober *prober, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, prober->id, io_probe, len, 1)) return;

    enum item item = args[0];
    if (!item_validate(item))
        return chunk_log(chunk, prober->id, io_scan, ioe_a0_invalid);

    struct coord coord = coord_nil();
    if (len >= 2) coord = coord_from_u64(args[1]);

    struct coord origin = chunk_star(chunk)->coord;
    if (coord_is_nil(coord)) coord = origin;

    prober->item = item;
    prober->coord = coord;

    uint8_t work = im_prober_work_cap(origin, coord);
    if (work == UINT8_MAX) {
        im_prober_reset(prober);
        return chunk_log(chunk, prober->id, io_scan, ioe_out_of_range);
    }

    prober->work.cap = prober->work.left = work;
    prober->result = im_prober_empty;
}

static void im_prober_io_value(
        struct im_prober *prober, struct chunk *chunk, im_id src)
{
    chunk_io(chunk, io_return, prober->id, src, &prober->result, 1);

    if (prober->result != im_prober_empty)
        im_prober_reset(prober);
}

static void im_prober_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_prober *prober = state;

    switch(io)
    {
    case io_ping: { chunk_io(chunk, io_pong, prober->id, src, NULL, 0); return; }
    case io_state: { im_prober_io_state(prober, chunk, src, args, len); return; }

    case io_probe: { im_prober_io_probe(prober, chunk, args, len); return; }
    case io_value: { im_prober_io_value(prober, chunk, src); return; }
    case io_reset: { im_prober_reset(prober); return; }

    default: { return; }
    }
}

static const struct io_cmd im_prober_io_list[] =
{
    { io_ping,  0, {} },
    { io_state, 1, { { "state", true } }},
    { io_reset, 0, {} },

    { io_probe, 2, { { "item", true },
                     { "coord", false } }},
    { io_value, 0, {} },
};
