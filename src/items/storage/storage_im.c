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

static void im_storage_init(void *state, struct chunk *chunk, id_t id)
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

static void im_storage_io_status(
        struct im_storage *storage, struct chunk *chunk, id_t src)
{
    word_t value = vm_pack(storage->count, storage->item);
    chunk_io(chunk, IO_STATE, storage->id, src, &value, 1);
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
        const word_t *args, size_t len)
{
    if (len < 1) return;

    enum item item = args[0];
    if (args[0] <= 0 || args[0] >= ITEM_MAX) return;
    if (!world_lab_known(chunk_world(chunk), item)) return;
    if (item == storage->item) return;

    im_storage_io_reset(storage, chunk);
    storage->item = item;
    storage->count = 0;
}

static void im_storage_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_storage *storage = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, storage->id, src, NULL, 0); return; }
    case IO_STATUS: { im_storage_io_status(storage, chunk, src); return; }

    case IO_ITEM: { im_storage_io_item(storage, chunk, args, len); return; }

    case IO_RESET: { im_storage_io_reset(storage, chunk); return; }

    default: { return; }
    }
}

static const word_t im_storage_io_list[] =
{
    IO_PING,
    IO_STATUS,

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

    *flow = (struct flow) {
        .id = storage->id,
        .loops = storage->count,
        .target = storage->item,
        .in = storage->item,
    };
    return true;
}
