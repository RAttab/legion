/* fusion_im.c
   Rémi Attab (remi.attab@gmail.com), 15 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "db/io.h"
#include "db/items.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// fusion
// -----------------------------------------------------------------------------

static void im_fusion_init(void *state, struct chunk *chunk, im_id id)
{
    (void) chunk;

    struct im_fusion *fusion = state;
    fusion->id = id;
    fusion->energy = im_fusion_energy_cap;
}


static void im_fusion_reset(struct im_fusion *fusion, struct chunk *chunk)
{
    fusion->paused = true;
    fusion->waiting = false;
    chunk_ports_reset(chunk, fusion->id);
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_fusion_step(void *state, struct chunk *chunk)
{
    struct im_fusion *fusion = state;
    if (fusion->paused) return;

    im_energy produced = legion_min(fusion->energy, im_fusion_energy_output);
    produced -= energy_step_fusion(
            chunk_energy(chunk), produced, im_fusion_energy_output);

    fusion->energy -= produced;

    if (!fusion->waiting) {
        if (fusion->energy + im_fusion_energy_rod <= im_fusion_energy_cap) {
            chunk_ports_request(chunk, fusion->id, im_fusion_input_item);
            fusion->waiting = true;
        }
        return;
    }

    enum item ret = chunk_ports_consume(chunk, fusion->id);
    if (!ret) return;

    fusion->waiting = false;
    fusion->energy += im_fusion_energy_rod;
    assert(fusion->energy <= im_fusion_energy_cap);
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_fusion_io_state(
        struct im_fusion *fusion, struct chunk *chunk, im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, fusion->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case io_active: { value = !fusion->paused; break; }
    case io_energy: { value = fusion->energy; break; }
    case io_item: {
        value = !fusion->paused && fusion->waiting ? im_fusion_input_item : 0;
        break;
    }
    default: { chunk_log(chunk, fusion->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, fusion->id, src, &value, 1);
}

static void im_fusion_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_fusion *fusion = state;

    switch(io)
    {
    case io_ping: { chunk_io(chunk, io_pong, fusion->id, src, NULL, 0); return; }
    case io_state: { im_fusion_io_state(fusion, chunk, src, args, len); return; }

    case io_reset: { im_fusion_reset(fusion, chunk); return; }
    case io_activate: { fusion->paused = false; return; }

    default: { return; }
    }
}

static const struct io_cmd im_fusion_io_list[] =
{
    { io_ping,     0, {} },
    { io_state,    1, { { "state", true } }},
    { io_reset,    0, {} },
    { io_activate, 0, { } },
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_fusion_flow(const void *state, struct flow *flow)
{
    const struct im_fusion *fusion = state;
    if (fusion->paused || !fusion->waiting) return false;

    *flow = (struct flow) {
        .id = fusion->id,
        .loops = 1,
        .target = item_energy,
        .item = im_fusion_input_item,
        .rank = tapes_info(im_fusion_input_item)->rank + 1,
    };
    return true;
}
