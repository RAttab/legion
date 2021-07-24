/* brain.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/io.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/active.h"
#include "vm/vm.h"
#include "vm/mod.h"

static void brain_mod(struct brain *brain, struct chunk *chunk, mod_t id);


// -----------------------------------------------------------------------------
// brain
// -----------------------------------------------------------------------------

enum
{
    brain_stack_s = 0,
    brain_stack_m = 1,
    brain_stack_l = 2,

    brain_speed_s = 2,
    brain_speed_m = 4,
    brain_speed_l = 6,

    brain_len_s = sizeof(struct brain) + vm_len(brain_stack_s),
    brain_len_m = sizeof(struct brain) + vm_len(brain_stack_m),
    brain_len_l = sizeof(struct brain) + vm_len(brain_stack_l),
};


static void brain_init(void *state, id_t id, struct chunk *chunk)
{
    struct brain *brain = state;
    (void) chunk;

    brain->id = id;

    switch (id_item(id)) {
    case ITEM_BRAIN_1: { vm_init(&brain->vm, brain_stack_s, brain_speed_s); break; }
    case ITEM_BRAIN_2: { vm_init(&brain->vm, brain_stack_m, brain_speed_s); break; }
    case ITEM_BRAIN_3: { vm_init(&brain->vm, brain_stack_l, brain_speed_s); break; }
    default: { assert(false); }
    }
}

static void brain_make(void *state, id_t id, struct chunk *chunk, uint32_t data)
{
    struct brain *brain = state;
    brain_init(brain, id, chunk);
    brain_mod(brain, chunk, data);
}

static void brain_load(void *state, struct chunk *chunk)
{
    struct brain *brain = state;
    if (!brain->mod_id) return;

    brain->mod = mods_get(world_mods(chunk_world(chunk)), brain->mod_id);
    assert(brain->mod);
}

static void brain_mod(struct brain *brain, struct chunk *chunk, mod_t id)
{
    const struct mod *mod = mods_get(world_mods(chunk_world(chunk)), id);
    if (!mod) return;

    brain->mod = mod;
    brain->mod_id = id;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static bool brain_step_recv(struct brain *brain, size_t len, const word_t *args)
{
    (void) args, (void) len;
    assert(brain->msg_len <= brain_msg_cap);

    for (size_t i = 0; i < brain->msg_len; ++i)
        vm_push(&brain->vm, brain->msg[brain->msg_len - i - 1]);
    vm_push(&brain->vm, vm_pack(brain->msg_src, brain->msg_len));

    brain->msg_src = 0;
    brain->msg_len = 0;
    return true;
}

static void brain_step_io(
        struct brain *brain, struct chunk *chunk, size_t len, const word_t *io)
{
    uint32_t atom = 0, dst = 0;
    vm_unpack(io[0], &atom, &dst);

    if (!dst) dst = brain->id;
    if (atom < (uint32_t) ATOM_IO_MIN || atom >= (uint32_t) ATOM_IO_MAX) {
        vm_io_fault(&brain->vm);
        return;
    }

    bool ok = false;
    switch (atom) {
    case IO_RECV: { ok = brain_step_recv(brain, len - 1, io + 1); break; }
    default: { ok = chunk_io(chunk, atom, brain->id, dst, len - 1, io + 1); break; }
    }

    // ALL IO operations must push a value on the stack to maintain our lisp
    // language invariant that all statements return a value on the stack.
    vm_push(&brain->vm, ok ? IO_OK : IO_FAIL);
}

static void brain_step(void *state, struct chunk *chunk)
{
    struct brain *brain = state;
    if (!brain->mod) return;

    mod_t mod = vm_exec(&brain->vm, brain->mod);
    if (mod == VM_FAULT) return;
    if (mod) { brain_mod(brain, chunk, mod); return; }

    if (!vm_io(&brain->vm)) return;
    vm_io_buf_t io = {0};
    size_t len = vm_io_read(&brain->vm, io);
    if (len) brain_step_io(brain, chunk, len, io);
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void brain_io_status(struct brain *brain, struct chunk *chunk, id_t src)
{
    word_t value = vm_pack(brain->msg_len, brain->mod_id);
    chunk_io(chunk, IO_STATE, brain->id, src, 1, &value);
}

static void brain_io_state(struct brain *brain, size_t len, const word_t *args)
{
    for (size_t i = 0; i < len; ++i)
        vm_push(&brain->vm,  args[i]);
}

static void brain_io_mod(
        struct brain *brain, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    mod_t id = args[0];
    if (id != args[0]) return;

    vm_reset(&brain->vm);
    brain_mod(brain, chunk, id);
}

static void brain_io_reset(struct brain *brain)
{
    brain->mod = NULL;
    brain->mod_id = 0;
    brain->msg_len = 0;
}

static void brain_io_val(struct brain *brain, size_t len, const word_t *args)
{
    (void) len;
    if (len < 1) return;

    vm_push(&brain->vm,  args[0]);
}

static void brain_io_send(
        struct brain *brain, id_t src, size_t len, const word_t *args)
{
    if (len > brain_msg_cap) return;

    brain->msg_src = src;
    brain->msg_len = len;
    memcpy(brain->msg, args, len * sizeof(*args));
}

static void brain_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct brain *brain = state;

    switch(io) {
    case IO_PING: { chunk_io(chunk, IO_PONG, brain->id, src, 0, NULL); return; }
    case IO_STATUS: { brain_io_status(brain, chunk, src); return; }
    case IO_STATE: { brain_io_state(brain, len, args); return; }
    case IO_MOD: { brain_io_mod(brain, chunk, len, args); return; }
    case IO_RESET: { brain_io_reset(brain); return; }
    case IO_VAL: { brain_io_val(brain, len, args); return; }
    case IO_SEND: { brain_io_send(brain, src, len, args); return; }
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct active_config *brain_config(enum item item)
{
    static const word_t io_list[] = {
        IO_PING, IO_STATUS, IO_STATE, IO_MOD, IO_RESET, IO_VAL, IO_SEND, IO_RECV,
    };

    switch(item) {

    case ITEM_BRAIN_1: {
        static const struct active_config config = {
            .size = brain_len_s,
            .init = brain_init,
            .make = brain_make,
            .load = brain_load,
            .step = brain_step,
            .io = brain_io,
            .io_list = io_list,
            .io_list_len = array_len(io_list),
        };
        return &config;
    }

    case ITEM_BRAIN_2: {
        static const struct active_config config = {
            .size = brain_len_m,
            .init = brain_init,
            .make = brain_make,
            .load = brain_load,
            .step = brain_step,
            .io = brain_io,
            .io_list = io_list,
            .io_list_len = array_len(io_list),
        };
        return &config;
    }

    case ITEM_BRAIN_3: {
        static const struct active_config config = {
            .size = brain_len_l,
            .init = brain_init,
            .make = brain_make,
            .load = brain_load,
            .step = brain_step,
            .io = brain_io,
            .io_list = io_list,
            .io_list_len = array_len(io_list),
        };
        return &config;
    }

    default: { assert(false); }
    }
}
