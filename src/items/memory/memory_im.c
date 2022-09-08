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

static void im_memory_init(void *state, struct chunk *chunk, id id)
{
    struct im_memory *memory = state;
    (void) chunk;

    memory->id = id;
    memory->len = im_memory_len(id_item(id));
}

static void im_memory_make(
        void *state, struct chunk *chunk, id id, const word_t *data, size_t len)
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

static void im_memory_io_state(
        struct im_memory *memory, struct chunk *chunk, id src,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, memory->id, IO_STATE, len, 1)) return;
    word_t value = 0;

    switch (args[0]) {
    default: { chunk_log(chunk, memory->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, memory->id, src, &value, 1);
}

static void im_memory_io_get(
        struct im_memory *memory, struct chunk *chunk,
        id src,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, memory->id, IO_GET, len, 1)) goto fail;

    uint8_t index = args[0];
    if (args[0] < 0 || args[0] >= memory->len) {
        chunk_log(chunk, memory->id, IO_GET, IOE_A0_INVALID);
        goto fail;
    }

    word_t value = memory->data[index];
    chunk_io(chunk, IO_RETURN, memory->id, src, &value, 1);
    return;

  fail:
    {
        word_t fail = 0;
        chunk_io(chunk, IO_RETURN, memory->id, src, &fail, 1);
    }
    return;
}

static void im_memory_io_set(
        struct im_memory *memory, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, memory->id, IO_SET, len, 2)) return;

    uint8_t index = args[0];
    if (args[0] < 0 || args[0] >= memory->len)
        return chunk_log(chunk, memory->id, IO_GET, IOE_A0_INVALID);

    memory->data[index] = args[1];
}

static void im_memory_io_cas(
        struct im_memory *memory, struct chunk *chunk,
        id src,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, memory->id, IO_CAS, len, 3)) goto fail;

    uint8_t index = args[0];
    if (args[0] < 0 || args[0] >= memory->len) {
        chunk_log(chunk, memory->id, IO_CAS, IOE_A0_INVALID);
        goto fail;
    }

    word_t exp = args[1];
    word_t val = args[2];
    word_t old = memory->data[index];
    if (old == exp) memory->data[index] = val;

    chunk_io(chunk, IO_RETURN, memory->id, src, &old, 1);
    return;

  fail:
    {
        word_t fail = 0;
        chunk_io(chunk, IO_RETURN, memory->id, src, &fail, 1);
    }
    return;
}

static void im_memory_io(
        void *state, struct chunk *chunk,
        enum io io, id src,
        const word_t *args, size_t len)
{
    struct im_memory *memory = state;

    switch (io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, memory->id, src, NULL, 0); return; }
    case IO_STATE: { im_memory_io_state(memory, chunk, src, args, len); return; }

    case IO_GET: { im_memory_io_get(memory, chunk, src, args, len); return; }
    case IO_SET: { im_memory_io_set(memory, chunk, args, len); return; }
    case IO_CAS: { im_memory_io_cas(memory, chunk, src, args, len); return; }

    default: { return; }
    }
}

static const word_t im_memory_io_list[] =
{
    IO_PING,
    IO_STATE,

    IO_GET,
    IO_SET,
    IO_CAS,
};
