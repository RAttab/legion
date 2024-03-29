/* memory_im.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// memory
// -----------------------------------------------------------------------------

static size_t im_memory_len(enum item type)
{
    switch (type) {
    case item_memory: { return im_memory_len_base; }
    default: { assert(false); }
    }
}

static void im_memory_init(void *state, struct chunk *chunk, im_id id)
{
    struct im_memory *memory = state;
    (void) chunk;

    memory->id = id;
    memory->len = im_memory_len(im_id_item(id));
}

static void im_memory_make(
        void *state, struct chunk *chunk,
        im_id id,
        const vm_word *data, size_t len)
{
    struct im_memory *memory = state;
    im_memory_init(memory, chunk, id);

    if (len < 1) return;

    len = legion_min(len, im_memory_len(im_id_item(id)));
    for (size_t i = 0; i < len; ++i)
        memory->data[i] = data[i];
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_memory_io_state(
        struct im_memory *memory, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, memory->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    default: { chunk_log(chunk, memory->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, memory->id, src, &value, 1);
}

static void im_memory_io_reset(struct im_memory *memory)
{
    memset(memory->data, 0, memory->len * sizeof(memory->data[0]));
}

static void im_memory_io_get(
        struct im_memory *memory, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, memory->id, io_get, len, 1)) goto fail;

    uint8_t index = args[0];
    if (args[0] < 0 || args[0] >= memory->len) {
        chunk_log(chunk, memory->id, io_get, ioe_a0_invalid);
        goto fail;
    }

    vm_word value = memory->data[index];
    chunk_io(chunk, io_return, memory->id, src, &value, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, memory->id, src, &fail, 1);
    }
    return;
}

static void im_memory_io_set(
        struct im_memory *memory, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, memory->id, io_set, len, 2)) return;

    uint8_t index = args[0];
    if (args[0] < 0 || args[0] >= memory->len)
        return chunk_log(chunk, memory->id, io_get, ioe_a0_invalid);

    memory->data[index] = args[1];
}

static void im_memory_io_cas(
        struct im_memory *memory, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, memory->id, io_cas, len, 3)) goto fail;

    uint8_t index = args[0];
    if (args[0] < 0 || args[0] >= memory->len) {
        chunk_log(chunk, memory->id, io_cas, ioe_a0_invalid);
        goto fail;
    }

    vm_word exp = args[1];
    vm_word val = args[2];
    vm_word old = memory->data[index];
    if (old == exp) memory->data[index] = val;

    chunk_io(chunk, io_return, memory->id, src, &old, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, memory->id, src, &fail, 1);
    }
    return;
}

static void im_memory_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_memory *memory = state;

    switch (io)
    {
    case io_ping: { chunk_io(chunk, io_pong, memory->id, src, NULL, 0); return; }
    case io_state: { im_memory_io_state(memory, chunk, src, args, len); return; }
    case io_reset: { im_memory_io_reset(memory); return; }

    case io_get: { im_memory_io_get(memory, chunk, src, args, len); return; }
    case io_set: { im_memory_io_set(memory, chunk, args, len); return; }
    case io_cas: { im_memory_io_cas(memory, chunk, src, args, len); return; }

    default: { return; }
    }
}

static const struct io_cmd im_memory_io_list[] =
{
    { io_ping,  0, {} },
    { io_state, 1, { { "state", true } }},
    { io_reset, 0, {} },

    { io_get,   1, { { "index", true } }},
    { io_set,   2, { { "index", true },
                     { "value", true } }},
    { io_cas,   3, { { "index", true },
                     { "expected", true },
                     { "value", true } }},
};
