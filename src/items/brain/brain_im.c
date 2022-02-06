/* brain_im.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/io.h"
#include "game/chunk.h"
#include "game/world.h"
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
    brain->breakpoint = IP_NIL;

    switch (id_item(id)) {
    case ITEM_BRAIN: { vm_init(&brain->vm, im_brain_stack_base, im_brain_speed_base); break; }
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
    case ITEM_BRAIN: { return vm_speed(im_brain_speed_base); }
    default: { assert(false); }
    }
}

static void im_brain_reset(struct im_brain *brain)
{
    brain->mod = NULL;
    brain->mod_id = 0;

    brain->debug = 0;
    brain->breakpoint = IP_NIL;

    brain->msg = (struct im_packet) {0};
    vm_reset(&brain->vm);
}

static void im_brain_recv(
        struct im_brain *brain, const word_t *args, size_t len)
{
    len = legion_min(len, (size_t) im_packet_max);

    for (size_t i = 0; i < len; ++i)
        vm_push(&brain->vm, args[len - i - 1]);
    vm_push(&brain->vm, len);
}

static void im_brain_name(
        struct im_brain *brain, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (len) chunk_rename(chunk, args[0]);
    else vm_push(&brain->vm, chunk_name(chunk));
}

static void im_brain_log(
        struct im_brain *brain, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, brain->id, IO_LOG, len, 2)) return;
    chunk_log(chunk, brain->id, args[0], args[1]);
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static bool im_brain_step_recv(struct im_brain *brain)
{
    if (!brain->msg.len) return false;
    im_brain_recv(brain, brain->msg.data, brain->msg.len);
    brain->msg = (struct im_packet) {0};
    return true;
}

static void im_brain_step_io(
        struct im_brain *brain, struct chunk *chunk, const word_t *io, size_t len)
{
    uint32_t atom = 0, dst = 0;
    vm_unpack(io[0], &atom, &dst);

    if (!dst) dst = brain->id;
    if (atom < (uint32_t) IO_MIN || atom >= (uint32_t) IO_MAX) {
        dbgf("brain.step.io: io=%lx, atom=%x, dst=%x", io[0], atom, dst);
        vm_io_fault(&brain->vm);
        return;
    }

    bool ok = true;
    switch (atom)
    {
    case IO_RECV: { ok = im_brain_step_recv(brain); break; }

    case IO_ID: { vm_push(&brain->vm, brain->id); break; }
    case IO_LOG: { im_brain_log(brain, chunk, io + 1, len - 1); break; }
    case IO_TICK: { vm_push(&brain->vm, world_time(chunk_world(chunk))); break; }
    case IO_COORD: { vm_push(&brain->vm, coord_to_u64(chunk_star(chunk)->coord)); break; }
    case IO_NAME: { im_brain_name(brain, chunk, io + 1, len - 1); break; }

    default: { ok = chunk_io(chunk, atom, brain->id, dst, io + 1, len - 1); break; }
    }

    // ALL IO operations must push a value on the stack to maintain our lisp
    // language invariant that all statements return a value on the stack.
    vm_push(&brain->vm, ok ? IO_OK : IO_FAIL);
}

static void im_brain_vm_step(struct im_brain *brain, struct chunk *chunk)
{
    if (!brain->mod || brain->mod_fault || vm_fault(&brain->vm)) return;

    mod_t mod = vm_exec(&brain->vm, brain->mod);
    if (brain->vm.ip == brain->breakpoint) brain->debug = true;

    if (mod == VM_FAULT)
        return chunk_log(chunk, brain->id, IO_STEP, IOE_VM_FAULT);

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

static void im_brain_io_state(
        struct im_brain *brain, struct chunk *chunk, id_t src,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, brain->id, IO_STATE, len, 1)) return;
    word_t value = 0;

    switch (args[0]) {
    case IO_MOD: { value = brain->mod_id; break; }
    case IO_DBG_BREAK: { value = brain->breakpoint; break; }
    default: { chunk_log(chunk, brain->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, brain->id, src, &value, 1);
}

static void im_brain_io_mod(
        struct im_brain *brain, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, brain->id, IO_MOD, len, 1)) return;

    mod_t id = args[0];
    if (!mod_validate(args[0]))
        return chunk_log(chunk, brain->id, IO_MOD, IOE_A0_INVALID);

    vm_reset(&brain->vm);
    im_brain_mod(brain, chunk, id);

    if (id && !brain->mod)
        return chunk_log(chunk, brain->id, IO_MOD, IOE_A0_UNKNOWN);
}

static void im_brain_io_return(
        struct im_brain *brain, struct chunk *chunk, const word_t *args, size_t len)
{
    if (!im_check_args(chunk, brain->id, IO_RETURN, len, 1)) return;
    vm_push(&brain->vm, args[0]);
}

static void im_brain_io_send(
        struct im_brain *brain, const word_t *args, size_t len)
{
    brain->msg.len = legion_min(len, (size_t) im_packet_max);
    memcpy(brain->msg.data, args, brain->msg.len * sizeof(*args));
}

static void im_brain_io_dbg_break(
        struct im_brain *brain, struct chunk *chunk, const word_t *args, size_t len)
{
    if (!im_check_args(chunk, brain->id, IO_DBG_BREAK, len, 1)) return;

    ip_t ip = args[0];
    if (!ip_validate(args[0]))
        return chunk_log(chunk, brain->id, IO_DBG_BREAK, IOE_A0_INVALID);

    brain->breakpoint = ip ? ip : IP_NIL;
    brain->vm.specs.speed = brain->breakpoint != IP_NIL ? 1 : im_brain_speed(brain);
}

static void im_brain_io_dbg_step(struct im_brain *brain, struct chunk *chunk)
{
    if (!brain->debug)
        return chunk_log(chunk, brain->id, IO_DBG_STEP, IOE_INVALID_STATE);

    uint8_t old = legion_xchg(&brain->vm.specs.speed, 1);
    im_brain_vm_step(brain, chunk);
    brain->vm.specs.speed = old;
}

static void im_brain_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_brain *brain = state;

    switch(io)
    {
    case IO_RETURN: { im_brain_io_return(brain, chunk, args, len); return; }

    case IO_PING: { chunk_io(chunk, IO_PONG, brain->id, src, NULL, 0); return; }
    case IO_PONG: { return; } // the return value of chunk_io is all we really need.

    case IO_STATE: { im_brain_io_state(brain, chunk, src, args, len); return; }
    case IO_RESET: { im_brain_reset(brain); return; }
    case IO_MOD: { im_brain_io_mod(brain, chunk, args, len); return; }

    case IO_LOG: { im_brain_log(brain, chunk, args, len); return; }
    case IO_NAME: { im_brain_name(brain, chunk, args, len); return; }

    case IO_SEND: { im_brain_io_send(brain, args, len); return; }
    case IO_RECV: { im_brain_recv(brain, args, len); return; }

    case IO_DBG_ATTACH: { brain->debug = true; return; }
    case IO_DBG_DETACH: { brain->debug = false; return; }
    case IO_DBG_BREAK: { im_brain_io_dbg_break(brain, chunk, args, len); return; }
    case IO_DBG_STEP: { im_brain_io_dbg_step(brain, chunk); return; }

    default: { return; }
    }
}

static const word_t im_brain_io_list[] =
{
    IO_RETURN,

    IO_PING,
    IO_PONG,

    IO_STATE,
    IO_RESET,
    IO_MOD,

    IO_ID,
    IO_LOG,
    IO_COORD,
    IO_NAME,

    IO_SEND,
    IO_RECV,

    IO_DBG_ATTACH,
    IO_DBG_DETACH,
    IO_DBG_BREAK,
    IO_DBG_STEP,
};
