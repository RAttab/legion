/* brain_im.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

static void im_brain_mod(struct im_brain *brain, struct chunk *chunk, mod_id id);


// -----------------------------------------------------------------------------
// brain
// -----------------------------------------------------------------------------


static void im_brain_init(void *state, struct chunk *chunk, im_id id)
{
    struct im_brain *brain = state;
    (void) chunk;

    brain->id = id;
    brain->breakpoint = vm_ip_nil;

    switch (im_id_item(id))
    {
    case item_brain: {
        vm_init(&brain->vm, im_brain_stack_base, im_brain_speed_base);
        break;
    }

    default: { assert(false); }
    }
}

static void im_brain_make(
        void *state, struct chunk *chunk,
        im_id id,
        const vm_word *data, size_t len)
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

static void im_brain_mod(struct im_brain *brain, struct chunk *chunk, mod_id id)
{
    brain->mod_id = id;
    brain->mod = id ? mods_get(world_mods(chunk_world(chunk)), id) : NULL;
    brain->fault = id && !brain->mod;
}

static uint8_t im_brain_speed(struct im_brain *brain)
{
    switch (im_id_item(brain->id)) {
    case item_brain: { return vm_speed(im_brain_speed_base); }
    default: { assert(false); }
    }
}

static void im_brain_reset(struct im_brain *brain)
{
    brain->mod = NULL;
    brain->mod_id = 0;

    brain->debug = 0;
    brain->breakpoint = vm_ip_nil;

    brain->msg = (struct im_packet) {0};
    vm_reset(&brain->vm);
}

static void im_brain_recv(
        struct im_brain *brain, const vm_word *args, size_t len)
{
    len = legion_min(len, (size_t) im_packet_max);

    for (size_t i = 0; i < len; ++i)
        vm_push(&brain->vm, args[len - i - 1]);
    vm_push(&brain->vm, len);
}

static void im_brain_log(
        struct im_brain *brain, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, brain->id, io_log, len, 2)) return;
    chunk_log(chunk, brain->id, args[0], args[1]);
}

static struct specs_ret im_brain_specs(
        struct im_brain *brain, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, brain->id, io_specs, len, 1))
        return (struct specs_ret) { .ok = false };

    enum spec spec = spec_from_word(args[0]);
    if (!spec_validate(args[0])) {
        chunk_log(chunk, brain->id, io_specs, ioe_a0_invalid);
        return (struct specs_ret) { .ok = false };
    }

    return specs_args(spec, args + 1, len - 1);
}

static void im_brain_return_value(
        struct im_brain *brain, struct chunk *chunk, im_id src, vm_word value)
{
    chunk_io(chunk, io_return, brain->id, src, &value, 1);
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_brain_step_name(
        struct im_brain *brain, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (len) chunk_rename(chunk, args[0]);
    else vm_push(&brain->vm, chunk_name(chunk));
}

static bool im_brain_step_specs(
        struct im_brain *brain, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    struct specs_ret ret = im_brain_specs(brain, chunk, args, len);
    if (ret.ok) vm_push(&brain->vm, ret.word);
    return ret.ok;
}

static bool im_brain_step_recv(struct im_brain *brain)
{
    im_brain_recv(brain, brain->msg.data, brain->msg.len);
    brain->msg = (struct im_packet) {0};
    return true;
}

// IO commands sent by the brain iteself.
static void im_brain_step_io(
        struct im_brain *brain, struct chunk *chunk, const vm_word *io, size_t len)
{
    uint32_t atom = 0, dst = 0;
    vm_unpack(io[0], &atom, &dst);

    if (!dst) dst = brain->id;
    if (atom < (uint32_t) io_min || atom >= (uint32_t) io_max) {
        chunk_log(chunk, brain->id, io_step, ioe_vm_fault);
        vm_io_fault(&brain->vm);
        return;
    }

    bool ok = true;
    switch (atom)
    {
    case io_recv: { ok = im_brain_step_recv(brain); break; }

    case io_id: { vm_push(&brain->vm, brain->id); break; }
    case io_log: { im_brain_log(brain, chunk, io + 1, len - 1); break; }
    case io_tick: { vm_push(&brain->vm, world_time(chunk_world(chunk))); break; }
    case io_coord: { vm_push(&brain->vm, coord_to_u64(chunk_star(chunk)->coord)); break; }
    case io_name: { im_brain_step_name(brain, chunk, io + 1, len - 1); break; }
    case io_specs: { ok = im_brain_step_specs(brain, chunk, io + 1, len - 1); break; }

    default: { ok = chunk_io(chunk, atom, brain->id, dst, io + 1, len - 1); break; }
    }

    // ALL IO operations must push a value on the stack to maintain our lisp
    // language invariant that all statements return a value on the stack.
    vm_push(&brain->vm, ok ? io_ok : io_fail);
}

static void im_brain_vm_step(struct im_brain *brain, struct chunk *chunk)
{
    if (!brain->mod || brain->fault || vm_fault(&brain->vm)) return;

    mod_id mod = vm_exec(&brain->vm, brain->mod);
    if (brain->vm.ip == brain->breakpoint) brain->debug = true;

    if (mod == VM_FAULT)
        return chunk_log(chunk, brain->id, io_step, ioe_vm_fault);

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

static void im_brain_io_return(
        struct im_brain *brain, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, brain->id, io_return, len, 1)) return;
    vm_push(&brain->vm, args[0]);
}

static void im_brain_io_state(
        struct im_brain *brain, struct chunk *chunk, im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, brain->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case io_mod: { value = brain->mod_id; break; }
    case io_dbg_break: { value = brain->breakpoint; break; }
    default: { chunk_log(chunk, brain->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, brain->id, src, &value, 1);
}

static void im_brain_io_mod(
        struct im_brain *brain, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, brain->id, io_mod, len, 1)) return;

    mod_id id = args[0];
    if (!mod_validate(args[0]))
        return chunk_log(chunk, brain->id, io_mod, ioe_a0_invalid);

    vm_reset(&brain->vm);
    im_brain_mod(brain, chunk, id);

    if (id && !brain->mod)
        return chunk_log(chunk, brain->id, io_mod, ioe_a0_unknown);
}

static void im_brain_io_name(
        struct im_brain *brain, struct chunk *chunk,
        im_id src, const vm_word *args, size_t len)
{
    if (len) chunk_rename(chunk, args[0]);
    else im_brain_return_value(brain, chunk, src, chunk_name(chunk));
}

static void im_brain_io_send(
        struct im_brain *brain, const vm_word *args, size_t len)
{
    brain->msg.len = legion_min(len, (size_t) im_packet_max);
    memcpy(brain->msg.data, args, brain->msg.len * sizeof(*args));
}

static void im_brain_io_dbg_break(
        struct im_brain *brain, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, brain->id, io_dbg_break, len, 1)) return;

    vm_ip ip = args[0];
    if (!ip_validate(args[0]))
        return chunk_log(chunk, brain->id, io_dbg_break, ioe_a0_invalid);

    brain->breakpoint = ip ? ip : vm_ip_nil;
    brain->vm.specs.speed = brain->breakpoint != vm_ip_nil ? 1 : im_brain_speed(brain);
}

static void im_brain_io_dbg_step(struct im_brain *brain, struct chunk *chunk)
{
    if (!brain->debug)
        return chunk_log(chunk, brain->id, io_dbg_step, ioe_invalid_state);

    uint8_t old = legion_xchg(&brain->vm.specs.speed, 1);
    im_brain_vm_step(brain, chunk);
    brain->vm.specs.speed = old;
}

// IO commands sent from another item.
static void im_brain_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_brain *brain = state;

    switch(io)
    {

    case io_return: { im_brain_io_return(brain, chunk, args, len); return; }
    case io_ping: { chunk_io(chunk, io_pong, brain->id, src, NULL, 0); return; }
    case io_pong: { return; } // the return value of chunk_io is all we really need.

    case io_state: { im_brain_io_state(brain, chunk, src, args, len); return; }
    case io_reset: { im_brain_reset(brain); return; }

    case io_id: { im_brain_return_value(brain, chunk, src, brain->id); break; }
    case io_name: { im_brain_io_name(brain, chunk, src, args, len); return; }
    case io_mod: { im_brain_io_mod(brain, chunk, args, len); return; }

    case io_tick: {
        vm_word value = world_time(chunk_world(chunk));
        im_brain_return_value(brain, chunk, src, value);
        break;
    }
    case io_coord: {
        vm_word value = coord_to_u64(chunk_star(chunk)->coord);
        im_brain_return_value(brain, chunk, src, value);
        break;
    }
    case io_specs: {
        vm_word value = im_brain_specs(brain, chunk, args, len).word;
        im_brain_return_value(brain, chunk, src, value);
        break;
    }
    case io_log: { im_brain_log(brain, chunk, args, len); return; }

    case io_send: { im_brain_io_send(brain, args, len); return; }
    case io_recv: { im_brain_recv(brain, args, len); return; }

    case io_dbg_attach: { brain->debug = true; return; }
    case io_dbg_detach: { brain->debug = false; return; }
    case io_dbg_break: { im_brain_io_dbg_break(brain, chunk, args, len); return; }
    case io_dbg_step: { im_brain_io_dbg_step(brain, chunk); return; }

    default: { return; }
    }
}

static const struct io_cmd im_brain_io_list[] =
{
    { io_ping,  0, {} },
    { io_state, 1, { { "state", true } }},
    { io_reset, 0, {} },

    { io_id,    0, {} },
    { io_name,  1, { { "name", false } }},
    { io_mod,   1, { { "mod-id", true } }},

    { io_tick,  0, {} },
    { io_coord, 0, {} },
    { io_specs, 1, { { "spec-id", true } }},
    { io_log,   2, { { "msg[0]", true },
                     { "msg[1]", true } }},

    { io_send,  5, { { "dst-id", true },
                     { "msg[0]", true },
                     { "msg[1]", false },
                     { "msg[2]", false },
                     { "msg[3]", false } }},
    { io_recv,  0, {} },

    { io_dbg_attach, 0, {} },
    { io_dbg_detach, 0, {} },
    { io_dbg_break,  1, { { "ip", false } }},
    { io_dbg_step,   0, {} },
};
