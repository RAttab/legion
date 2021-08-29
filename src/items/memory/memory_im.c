/* memory_im.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/chunk.h"
#include "items/io.h"


// -----------------------------------------------------------------------------
// memory
// -----------------------------------------------------------------------------

static size_t im_memory_len(enum item type)
{
    switch (type) {
    case ITEM_MEMORY: { return im_memory_len_base; }
    default: { assert(false); }
    }
}

static void im_memory_init(void *state, struct chunk *chunk, id_t id)
{
    struct im_memory *memory = state;
    (void) chunk;

    memory->id = id;
    memory->len = im_memory_len(id_item(id));
}

static void im_memory_make(
        void *state, struct chunk *chunk, id_t id, const word_t *data, size_t len)
{
    struct im_memory *memory = state;
    im_memory_init(memory, chunk, id);

    if (len < 1) return;

    len = legion_min(len, im_memory_len(id_item(id)));
    for (size_t i = 0; i < len; ++i)
        memory->data[i] = data[i];
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_memory_io_status(struct im_memory *memory, struct chunk *chunk, id_t src)
{
    word_t value = memory->len;
    chunk_io(chunk, IO_STATE, memory->id, src, &value, 1);
}

static void im_memory_io_get(
        struct im_memory *memory, struct chunk *chunk,
        id_t src,
        const word_t *args, size_t len)
{
    word_t val = 0;
    if (len >= 1) {
        word_t index = args[0];
        if (index >= 0 && index < memory->len) val = memory->data[index];
    }
    chunk_io(chunk, IO_RETURN, memory->id, src, &val, 1);
}

static void im_memory_io_set(struct im_memory *memory, const word_t *args, size_t len)
{
    if (len < 2) return;

    if (args[0] >= memory->len) return;
    uint8_t index = args[0];

    memory->data[index] = args[1];
}

static void im_memory_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_memory *memory = state;

    switch (io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, memory->id, src, NULL, 0); return; }
    case IO_STATUS: { im_memory_io_status(memory, chunk, src); return; }

    case IO_GET: { im_memory_io_get(memory, chunk, src, args, len); return; }
    case IO_SET: { im_memory_io_set(memory, args, len); return; }

    default: { return; }
    }
}

static const word_t im_memory_io_list[] =
{
    IO_PING,
    IO_STATUS,

    IO_GET,
    IO_SET,
};
