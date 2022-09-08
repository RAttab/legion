/* storage_im.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "items/io.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// storage
// -----------------------------------------------------------------------------

static void im_storage_init(void *state, struct chunk *chunk, id id)
{
    struct im_storage *storage = state;
    (void) chunk;

    storage->id = id;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_storage_step(void *state, struct chunk *chunk)
{
    struct im_storage *storage = state;
    if (!storage->item) return;

    enum item ret = 0;
    if (!storage->waiting) {
        if (storage->count < im_storage_max) {
            chunk_ports_request(chunk, storage->id, storage->item);
            storage->waiting = true;
        }
    }
    else if ((ret = chunk_ports_consume(chunk, storage->id))){
        assert(ret == storage->item);
        storage->count++;
        storage->waiting = false;
    }

    if (storage->count && chunk_ports_produce(chunk, storage->id, storage->item))
        storage->count--;
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_storage_io_state(
        struct im_storage *storage, struct chunk *chunk, id src,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, storage->id, IO_STATE, len, 1)) return;
    word value = 0;

    switch (args[0]) {
    case IO_ITEM: { value = storage->item; break; }
    case IO_LOOP: { value = storage->count; break; }
    default: { chunk_log(chunk, storage->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, storage->id, src, &value, 1);
}

static void im_storage_io_reset(struct im_storage *storage, struct chunk *chunk)
{
    chunk_ports_reset(chunk, storage->id);
    storage->item = 0;
    storage->count = 0;
    storage->waiting = false;
}

static void im_storage_io_item(
        struct im_storage *storage, struct chunk *chunk,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, storage->id, IO_ITEM, len, 1)) return;

    enum item item = args[0];
    if (!item_validate(args[0]))
        return chunk_log(chunk, storage->id, IO_ITEM, IOE_A0_INVALID);

    if (!im_check_known(chunk, storage->id, IO_ITEM, item)) return;

    if (item == storage->item) return;
    im_storage_io_reset(storage, chunk);
    storage->item = item;
}

static void im_storage_io(
        void *state, struct chunk *chunk,
        enum io io, id src,
        const word *args, size_t len)
{
    struct im_storage *storage = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, storage->id, src, NULL, 0); return; }
    case IO_STATE: { im_storage_io_state(storage, chunk, src, args, len); return; }

    case IO_ITEM: { im_storage_io_item(storage, chunk, args, len); return; }

    case IO_RESET: { im_storage_io_reset(storage, chunk); return; }

    default: { return; }
    }
}

static const word im_storage_io_list[] =
{
    IO_PING,
    IO_STATE,

    IO_ITEM,
    IO_RESET,
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_storage_flow(const void *state, struct flow *flow)
{
    const struct im_storage *storage = state;
    if (!storage->item) return false;

    const struct tape_info *info = tapes_info(storage->item);
    if (!info) return false;

    *flow = (struct flow) {
        .id = storage->id,
        .loops = storage->count,
        .target = storage->item,
        .rank = info->rank,
    };

    if (!storage->count)
        flow->in = storage->item;
    else flow->out = storage->item;

    return true;
}
