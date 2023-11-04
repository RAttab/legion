/* packer_im.c
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "db/io.h"
#include "db/items.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// packer
// -----------------------------------------------------------------------------

static void im_packer_init(void *state, struct chunk *chunk, im_id id)
{
    (void) chunk;

    struct im_packer *packer = state;
    packer->id = id;
}


static void im_packer_reset(struct im_packer *packer, struct chunk *chunk)
{
    chunk_ports_reset(chunk, packer->id);
    legion_zero_from(packer, item);
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_packer_step(void *state, struct chunk *chunk)
{
    struct im_packer *packer = state;
    if (!packer->item) return;

    if (!packer->waiting) {
        im_id id = chunk_last(chunk, packer->item);
        if (!id) { im_packer_reset(packer, chunk); return; }

        bool ok = chunk_delete(chunk, id);
        assert(ok);

        chunk_ports_produce(chunk, packer->id, packer->item);
        packer->waiting = true;
        return;
    }

    if (!chunk_ports_consumed(chunk, packer->id)) return;
    packer->waiting = false;
    if (packer->loops != im_loops_inf) --packer->loops;
    if (!packer->loops) im_packer_reset(packer, chunk);
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_packer_io_state(
        struct im_packer *packer, struct chunk *chunk, im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, packer->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case io_item: { value = packer->item; break; }
    case io_loop: { value = packer->loops; break; }
    default: { chunk_log(chunk, packer->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, packer->id, src, &value, 1);
}


static void im_packer_io_id(
        struct im_packer *packer, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, packer->id, io_id, len, 1)) return;

    im_id id = args[0];
    enum item item = im_id_item(id);

    if (!im_id_validate(args[0]))
        return chunk_log(chunk, packer->id, io_id, ioe_a0_invalid);

    if (!item_is_active(item) && !item_is_logistics(item))
        return chunk_log(chunk, packer->id, io_id, ioe_a0_invalid);

    if (!im_check_known(chunk, packer->id, io_id, item)) return;

    if (!chunk_delete(chunk, id))
        return chunk_log(chunk, packer->id, io_id, ioe_a0_invalid);

    chunk_ports_produce(chunk, packer->id, item);
    packer->waiting = true;
    packer->item = item;
    packer->loops = 1;
}

static void im_packer_io_item(
        struct im_packer *packer, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, packer->id, io_item, len, 1)) return;

    enum item item = args[0];

    if (!item_validate(args[0]))
        return chunk_log(chunk, packer->id, io_item, ioe_a0_invalid);

    if (!item_is_active(item) && !item_is_logistics(item))
        return chunk_log(chunk, packer->id, io_item, ioe_a0_invalid);

    if (!im_check_known(chunk, packer->id, io_item, item)) return;

    im_packer_reset(packer, chunk);
    packer->item = item;
    packer->loops = im_loops_io(len > 1 ? args[1] : im_loops_inf);
}

static void im_packer_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_packer *packer = state;

    switch(io)
    {
    case io_ping: { chunk_io(chunk, io_pong, packer->id, src, NULL, 0); return; }
    case io_state: { im_packer_io_state(packer, chunk, src, args, len); return; }

    case io_id: { im_packer_io_id(packer, chunk, args, len); return; }
    case io_item: { im_packer_io_item(packer, chunk, args, len); return; }
    case io_reset: { im_packer_reset(packer, chunk); return; }

    default: { return; }
    }
}

static const struct io_cmd im_packer_io_list[] =
{
    { io_ping,  0, {} },
    { io_state, 1, { { "state", true } }},
    { io_reset, 0, {} },

    { io_id,     1, { { "pack-id", true } }},
    { io_pack,   2, { { "item", true },
                      { "loops", false } }},
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_packer_flow(const void *state, struct flow *flow)
{
    const struct im_packer *packer = state;
    if (!packer->item) return false;

    *flow = (struct flow) {
        .id = packer->id,
        .loops = packer->loops,
        .target = packer->item,
        .state = tape_output,
        .item = packer->item,
        .rank = tapes_info(packer->item)->rank + 1,
    };
    return true;
}
