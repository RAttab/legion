/* storage.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/io.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/active.h"


// -----------------------------------------------------------------------------
// storage
// -----------------------------------------------------------------------------

static void storage_init(void *state, id_t id, struct chunk *chunk)
{
    struct storage *storage = state;
    (void) chunk;

    storage->id = id;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void storage_step(void *state, struct chunk *chunk)
{
    struct storage *storage = state;
    if (!storage->item) return;

    enum item ret = 0;
    if (!storage->waiting) {
        if (storage->count < UINT16_MAX) {
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

static void storage_io_status(struct storage *storage, struct chunk *chunk, id_t src)
{
    word_t value = vm_pack(storage->count, storage->item);
    chunk_io(chunk, IO_STATE, storage->id, src, 1, &value);
}

static void storage_io_reset(struct storage *storage, struct chunk *chunk)
{
    chunk_ports_reset(chunk, storage->id);
    storage->item = 0;
    storage->count = 0;
    storage->waiting = false;
}

static void storage_io_item(
        struct storage *storage, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    word_t item = args[0];
    if (item != (enum item) item) return;
    if (item == storage->item) return;

    storage_io_reset(storage, chunk);
    storage->item = item;
    storage->count = 0;
}

static void storage_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct storage *storage = state;

    switch(io) {
    case IO_PING: { chunk_io(chunk, IO_PONG, storage->id, src, 0, NULL); return; }
    case IO_STATUS: { storage_io_status(storage, chunk, src); return; }
    case IO_ITEM: { storage_io_item(storage, chunk, len, args); return; }
    case IO_RESET: { storage_io_reset(storage, chunk); return; }
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct active_config *storage_config(enum item item)
{
    (void) item;
    static const word_t io_list[] = { IO_PING, IO_STATUS, IO_ITEM, IO_RESET };

    static const struct active_config config = {
        .size = sizeof(struct storage),
        .init = storage_init,
        .step = storage_step,
        .io = storage_io,
        .io_list = io_list,
        .io_list_len = array_len(io_list),
    };
    return &config;
}
