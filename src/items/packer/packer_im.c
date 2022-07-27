/* packer_im.c
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/io.h"
#include "items/item.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// packer
// -----------------------------------------------------------------------------

static void im_packer_init(void *state, struct chunk *chunk, id_t id)
{
    (void) chunk;

    struct im_packer *packer = state;
    packer->id = id;
}


static void im_packer_reset(struct im_packer *packer, struct chunk *chunk)
{
    chunk_ports_reset(chunk, packer->id);
    packer->item = 0;
    packer->loops = 0;
    packer->waiting = false;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_packer_step(void *state, struct chunk *chunk)
{
    struct im_packer *packer = state;
    if (!packer->item) return;

    if (!packer->waiting) {
        id_t id = chunk_last(chunk, packer->item);
        if (!id) { packer_reset(packer, chunk); return; }

        bool ok = chunk_delete(chunk, id);
        assert(ok);

        chunk_ports_produce(chunk, packer->item);
        packer->waiting = true;
        return;
    }

    if (!chunk_ports_consumed(chunk, packer->id)) return;
    packer->waiting = false;
    if (packer->loops != loops_inf) --packer->loops;
    if (!packer->loops) im_packer_reset(packer, chunk);
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_packer_io_state(
        struct im_packer *packer, struct chunk *chunk, id_t src,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, packer->id, IO_STATE, len, 1)) return;
    word_t value = 0;

    switch (args[0]) {
    case IO_ITEM: { value = packer->item; break; }
    case IO_LOOP: { value = packer->loops; break; }
    default: { chunk_log(chunk, packer->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, packer->id, src, &value, 1);
}


static void im_packer_io_id(
        struct im_packer *packer, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, packer->id, IO_ID, len, 1)) return;

    id_t id = args[0];
    enum item item = id_item(id);

    if (!item_validate(args[0]))
        return chunk_log(chunk, packer->id, IO_ID, IOE_A0_INVALID);

    if (!item_is_active(item) && !item_is_logistics(item))
        return chunk_log(chunk, packer->id, IO_ID, IOE_A0_INVALID);

    if (!im_check_known(chunk, packer->id, IO_ID, item)) return;

    if (!chunk_delete(chunk, id))
        return chunk_log(chunk, packer->id, IO_ID, IOE_A0_INVALID);

    chunk_ports_produce(chunk, packer->id, item);
    packer->waiting = true;
    packer->item = item;
    packer->loops = 1;
}

static void im_packer_io_item(
        struct im_packer *packer, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, packer->id, IO_ITEM, len, 1)) return;

    enum item item = args[0];

    if (!item_validate(args[0]))
        return chunk_log(chunk, packer->id, IO_ITEM, IOE_A0_INVALID);

    if (!item_is_active(item) && !item_is_logistics(item))
        return chunk_log(chunk, packer->id, IO_ITEM, IOE_A0_INVALID);

    if (!im_check_known(chunk, packer->id, IO_ITEM, item)) return;

    im_packer_reset(packer, chunk);
    packer->item = item;
    packer->loops = loops_io(len > 1 ? args[1] : loops_inf);
}

static void im_packer_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_packer *packer = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, packer->id, src, NULL, 0); return; }
    case IO_STATE: { im_packer_io_state(packer, chunk, src, args, len); return; }

    case IO_ID: { im_packer_io_id(packer, chunk, args, len); return; }
    case IO_ITEM: { im_packer_io_item(packer, chunk, args, len); return; }
    case IO_RESET: { im_packer_reset(packer, chunk); return; }

    default: { return; }
    }
}

static const word_t im_packer_io_list[] =
{
    IO_PING,
    IO_STATE,
    IO_RESET

    IO_ID,
    IO_ITEM,
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
        .out = packer->item,
        .rank = tapes_info(packer->item)->rank + 1,
    };
    return true;
}
