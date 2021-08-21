/* brain_im.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/io.h"
#include "game/chunk.h"
#include "vm/op.h"

static void im_brain_mod(struct im_brain *brain, struct chunk *chunk, mod_t id);


// -----------------------------------------------------------------------------
// brain
// -----------------------------------------------------------------------------


static void im_brain_init(void *state, struct chunk *chunk, id_t id)
{
    struct im_brain *brain = state;
    (void) chunk;

    brain->id = id;

    switch (id_item(id)) {
    case ITEM_BRAIN_1: { vm_init(&brain->vm, im_brain_stack_s, im_brain_speed_s); break; }
    case ITEM_BRAIN_2: { vm_init(&brain->vm, im_brain_stack_m, im_brain_speed_s); break; }
    case ITEM_BRAIN_3: { vm_init(&brain->vm, im_brain_stack_l, im_brain_speed_s); break; }
    default: { assert(false); }
    }
}

static void im_brain_make(
        void *state, struct chunk *chunk, id_t id, const word_t *data, size_t len)
{
    struct im_brain *brain = state;
    im_brain_init(brain, chunk, id);

    if (len < 1) return;
    if (data[0] <= 0 || data[0] > UINT32_MAX) return;

    im_brain_mod(brain, chunk, data[0]);
}

static void im_brain_load(void *state, struct chunk *chunk)
{
    struct im_brain *brain = state;
    if (!brain->mod_id) return;

    brain->mod = mods_get(world_mods(chunk_world(chunk)), brain->mod_id);
    assert(brain->mod);
}

static void im_brain_mod(struct im_brain *brain, struct chunk *chunk, mod_t id)
{
    brain->mod_id = id;
    brain->mod = id ? mods_get(world_mods(chunk_world(chunk)), id) : NULL;
    brain->mod_fault = id && !brain->mod;
}

static uint8_t im_brain_speed(struct im_brain *brain)
{
    switch (id_item(brain->id)) {
    case ITEM_BRAIN_1: { return vm_speed(im_brain_speed_s); }
    case ITEM_BRAIN_2: { return vm_speed(im_brain_speed_m); }
    case ITEM_BRAIN_3: { return vm_speed(im_brain_speed_l); }
    default: { assert(false); }
    }
}

static void im_brain_reset(struct im_brain *brain)
{
    brain->debug = 0;
    brain->breakpoint = 0;

    brain->msg_len = 0;

    brain->mod = NULL;
    brain->mod_id = 0;

    vm_reset(&brain->vm);
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static bool im_brain_step_recv(struct im_brain *brain, const word_t *args, size_t len)
{
    (void) args, (void) len;
    assert(brain->msg_len <= im_brain_msg_cap);

    for (size_t i = 0; i < brain->msg_len; ++i)
        vm_push(&brain->vm, brain->msg[brain->msg_len - i - 1]);
    vm_push(&brain->vm, vm_pack(brain->msg_src, brain->msg_len));

    brain->msg_src = 0;
    brain->msg_len = 0;
    return true;
}

static void im_brain_step_io(
        struct im_brain *brain, struct chunk *chunk, const word_t *io, size_t len)
{
    uint32_t atom = 0, dst = 0;
    vm_unpack(io[0], &atom, &dst);

    if (!dst) dst = brain->id;
    if (atom < (uint32_t) IO_MIN || atom >= (uint32_t) IO_MAX) {
        vm_io_fault(&brain->vm);
        return;
    }

    bool ok = true;
    switch (atom) {
    case IO_RECV: { ok = im_brain_step_recv(brain, io + 1, len - 1); break; }
    case IO_COORD: { vm_push(&brain->vm, coord_to_u64(chunk_star(chunk)->coord)); break; }
    default: { ok = chunk_io(chunk, atom, brain->id, dst, io + 1, len - 1); break; }
    }

    // ALL IO operations must push a value on the stack to maintain our lisp
    // language invariant that all statements return a value on the stack.
    vm_push(&brain->vm, ok ? IO_OK : IO_FAIL);
}

static void im_brain_vm_step(struct im_brain *brain, struct chunk *chunk)
{
    if (!brain->mod || brain->mod_fault) return;

    mod_t mod = vm_exec(&brain->vm, brain->mod);
    if (brain->vm.ip == brain->breakpoint) brain->debug = true;

    if (mod == VM_FAULT) return;
    if (mod == VM_RESET) { im_brain_reset(brain); return; }
    if (mod) { im_brain_mod(brain, chunk, mod); return; }

    if (vm_io(&brain->vm)) {
        vm_io_buf_t io = {0};
        size_t len = vm_io_read(&brain->vm, io);
        if (len) im_brain_step_io(brain, chunk, io, len);
    }
}

static void im_brain_step(void *state, struct chunk *chunk)
{
    struct im_brain *brain = state;
    if (brain->debug) return;

    im_brain_vm_step(brain, chunk);
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_brain_io_status(struct im_brain *brain, struct chunk *chunk, id_t src)
{
    word_t value = vm_pack(brain->msg_len, brain->mod_id);
    chunk_io(chunk, IO_STATE, brain->id, src, &value, 1);
}

static void im_brain_io_state(struct im_brain *brain, const word_t *args, size_t len)
{
    for (size_t i = 0; i < len; ++i)
        vm_push(&brain->vm, args[i]);
}

static void im_brain_io_mod(
        struct im_brain *brain, struct chunk *chunk, const word_t *args, size_t len)
{
    if (len < 1) return;

    mod_t id = args[0];
    if (id != args[0]) return;

    vm_reset(&brain->vm);
    im_brain_mod(brain, chunk, id);
}

static void im_brain_io_val(struct im_brain *brain, const word_t *args, size_t len)
{
    (void) len;
    if (len < 1) return;

    vm_push(&brain->vm,  args[0]);
}

static void im_brain_io_send(
        struct im_brain *brain, id_t src, const word_t *args, size_t len)
{
    if (len > im_brain_msg_cap) return;

    brain->msg_src = src;
    brain->msg_len = len;
    memcpy(brain->msg, args, len * sizeof(*args));
}

static void im_brain_io_dbg_break(struct im_brain *brain, const word_t *args, size_t len)
{
    if (len < 1) return;
    if (args[0] > UINT32_MAX || args[0] < 0) return;

    brain->breakpoint = args[0] ? args[0] : IP_NIL;
    brain->vm.specs.speed = brain->breakpoint != IP_NIL ? 1 : im_brain_speed(brain);
}

static void im_brain_io_dbg_step(struct im_brain *brain, struct chunk *chunk)
{
    if (!brain->debug) return;

    brain->vm.specs.speed = 1;
    im_brain_vm_step(brain, chunk);
    brain->vm.specs.speed = im_brain_speed(brain);
}

static void im_brain_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_brain *brain = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, brain->id, src, NULL, 0); return; }
    case IO_PONG: { return; } // the return value of chunk_io is all we really need.

    case IO_STATUS: { im_brain_io_status(brain, chunk, src); return; }
    case IO_STATE: { im_brain_io_state(brain, args, len); return; }

    case IO_MOD: { im_brain_io_mod(brain, chunk, args, len); return; }
    case IO_RESET: { im_brain_reset(brain); return; }

    case IO_VAL: { im_brain_io_val(brain, args, len); return; }
    case IO_SEND: { im_brain_io_send(brain, src, args, len); return; }

    case IO_DBG_ATTACH: { brain->debug = true; return; }
    case IO_DBG_DETACH: { brain->debug = false; return; }
    case IO_DBG_BREAK: { im_brain_io_dbg_break(brain, args, len); return; }
    case IO_DBG_STEP: { im_brain_io_dbg_step(brain, chunk); return; }

    default: { return; }
    }
}

static const word_t im_brain_io_list[] =
{
    IO_PING,
    IO_PONG,

    IO_STATUS,
    IO_STATE,

    IO_COORD,

    IO_MOD,
    IO_RESET,

    IO_VAL,
    IO_SEND,
    IO_RECV,

    IO_DBG_ATTACH,
    IO_DBG_DETACH,
    IO_DBG_BREAK,
    IO_DBG_STEP,
};