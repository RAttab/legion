/* fusion_im.c
   Rémi Attab (remi.attab@gmail.com), 15 Oct 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/io.h"
#include "items/item.h"
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
    fusion->energy = im_fusion_energy_rod;
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
        if (fusion->energy + im_fusion_energy_rod < im_fusion_energy_cap) {
            chunk_ports_request(chunk, fusion->id, im_fusion_input_item);
            fusion->waiting = true;
        }
        return;
    }

    enum item ret = chunk_ports_consume(chunk, fusion->id);
    if (!ret) return;

    fusion->energy += im_fusion_energy_rod;
    fusion->waiting = false;

    assert(fusion->energy < im_fusion_energy_cap);
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_fusion_io_state(
        struct im_fusion *fusion, struct chunk *chunk, im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, fusion->id, IO_STATE, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case IO_ACTIVE: { value = !fusion->paused; break; }
    case IO_ENERGY: { value = fusion->energy; break; }
    case IO_ITEM: {
        value = !fusion->paused && fusion->waiting ? im_fusion_input_item : 0;
        break;
    }
    default: { chunk_log(chunk, fusion->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, fusion->id, src, &value, 1);
}

static void im_fusion_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_fusion *fusion = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, fusion->id, src, NULL, 0); return; }
    case IO_STATE: { im_fusion_io_state(fusion, chunk, src, args, len); return; }

    case IO_RESET: { im_fusion_reset(fusion, chunk); return; }
    case IO_ACTIVATE: { fusion->paused = false; return; }

    default: { return; }
    }
}

static const struct io_cmd im_fusion_io_list[] =
{
    { IO_PING,     0, {} },
    { IO_STATE,    1, { { "state", true } }},
    { IO_RESET,    0, {} },
    { IO_ACTIVATE, 0, { } },
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_fusion_flow(const void *state, struct flow *flow)
{
    const struct im_fusion *fusion = state;
    if (fusion->paused) return false;

    *flow = (struct flow) {
        .id = fusion->id,
        .loops = fusion->waiting ? 1 : 0,
        .target = ITEM_ENERGY,
        .item = fusion->waiting ? im_fusion_input_item : 0,
        .rank = tapes_info(im_fusion_input_item)->rank + 1,
    };
    return true;
}
