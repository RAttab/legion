/* storage_im.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "db/io.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// storage
// -----------------------------------------------------------------------------

static void im_storage_init(void *state, struct chunk *chunk, im_id id)
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
        struct im_storage *storage, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, storage->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case io_item: { value = storage->item; break; }
    case io_loop: { value = storage->count; break; }
    default: { chunk_log(chunk, storage->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, storage->id, src, &value, 1);
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
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, storage->id, io_item, len, 1)) return;

    enum item item = args[0];
    if (!item_validate(args[0]))
        return chunk_log(chunk, storage->id, io_item, ioe_a0_invalid);

    if (!im_check_known(chunk, storage->id, io_item, item)) return;

    if (item == storage->item) return;
    im_storage_io_reset(storage, chunk);
    storage->item = item;
}

static void im_storage_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_storage *storage = state;

    switch(io)
    {
    case io_ping: { chunk_io(chunk, io_pong, storage->id, src, NULL, 0); return; }
    case io_state: { im_storage_io_state(storage, chunk, src, args, len); return; }

    case io_item: { im_storage_io_item(storage, chunk, args, len); return; }

    case io_reset: { im_storage_io_reset(storage, chunk); return; }

    default: { return; }
    }
}

static const struct io_cmd im_storage_io_list[] =
{
    { io_ping,  0, {} },
    { io_state, 1, { { "state", true } }},
    { io_reset, 0, {} },
    { io_item,  1, { { "item", true } }},
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
        .loops = legion_min(storage->count, im_loops_inf),
        .target = storage->item,
        .state = storage->count ? tape_output : tape_input,
        .item = storage->item,
        .rank = info->rank,
    };

    return true;
}
