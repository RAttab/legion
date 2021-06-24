/* brain.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/atoms.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/active.h"
#include "vm/vm.h"
#include "vm/mod.h"


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

    brain_len_s = sizeof(struct brain) + brain_stack_s * sizeof(word_t),
    brain_len_m = sizeof(struct brain) + brain_stack_m * sizeof(word_t),
    brain_len_l = sizeof(struct brain) + brain_stack_l * sizeof(word_t),
};


static void brain_init(void *state, id_t id, struct chunk *chunk)
{
    struct brain *brain = state;
    (void) chunk;

    brain->id = id;

    switch (id_item(id)) {
    case ITEM_BRAIN_S: { vm_init(&brain->vm, brain_stack_s, brain_speed_s); break; }
    case ITEM_BRAIN_M: { vm_init(&brain->vm, brain_stack_m, brain_speed_s); break; }
    case ITEM_BRAIN_L: { vm_init(&brain->vm, brain_stack_l, brain_speed_s); break; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void brain_step_recv(struct brain *brain, size_t len, const word_t *args)
{
    (void) args;

    if (len != 0) { vm_io_fault(&brain->vm); return; }
    assert(brain->msg_len <= brain_msg_cap);

    word_t header = vm_pack(brain->msg_src, brain->msg_len);
    vm_io_write(&brain->vm, brain->msg_len, brain->msg);
    vm_io_write(&brain->vm, 1, &header);

    brain->msg_src = 0;
    brain->msg_len = 0;
}

static void brain_step(void *state, struct chunk *chunk)
{
    struct brain *brain = state;
    if (!brain->mod) return;

    ip_t ret = vm_exec(&brain->vm, brain->mod);
    if (ret == VM_FAULT || !vm_io(&brain->vm)) return;

    vm_io_buf_t io = {0};
    size_t len = vm_io_read(&brain->vm, io);
    if (!len) return;

    switch (io[0]) {

    case IO_RECV: { brain_step_recv(brain, len - 1, io + 1); return; }

    default: {
        uint32_t atom = 0, dst = 0;
        vm_unpack(io[0], &atom, &dst);

        if (atom < (uint32_t) ATOM_IO_MIN || atom >= (uint32_t) ATOM_IO_MAX) {
            vm_io_fault(&brain->vm);
            return;
        }

        if (!dst) dst = brain->id;
        chunk_io(chunk, atom, brain->id, dst, len - 1, io + 1);
    }

    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void brain_io_prog(struct brain *brain, size_t len, const word_t *args)
{
    if (len < 1) return;

    mod_t id = args[0];
    if (id != args[0]) return;

    const struct mod *mod = mods_load(id);
    if (!mod) return;

    vm_reset(&brain->vm);
    brain->mod = mod;
}

static void brain_io_reset(struct brain *brain)
{
    vm_reset(&brain->vm);
    brain->mod = NULL;
    brain->msg_len = 0;
}

static void brain_io_val(struct brain *brain, size_t len, const word_t *args)
{
    vm_io_write(&brain->vm, len, args);
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
    case IO_PROG: { brain_io_prog(brain, len, args); return; }
    case IO_RESET: { brain_io_reset(brain); return; }
    case IO_VAL: { brain_io_val(brain, len, args); return; }
    case IO_SEND: { brain_io_send(brain, src, len, args); return; }
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct item_config *brain_config(item_t item)
{
    switch(item) {

    case ITEM_BRAIN_S: {
        static const struct item_config config = {
            .size = brain_len_s,
            .init = brain_init,
            .step = brain_step,
            .io = brain_io,
        };
        return &config;
    }

    case ITEM_BRAIN_M: {
        static const struct item_config config = {
            .size = brain_len_m,
            .init = brain_init,
            .step = brain_step,
            .io = brain_io,
        };
        return &config;
    }

    case ITEM_BRAIN_L: {
        static const struct item_config config = {
            .size = brain_len_l,
            .init = brain_init,
            .step = brain_step,
            .io = brain_io,
        };
        return &config;
    }

    default: { assert(false); }
    }
}
