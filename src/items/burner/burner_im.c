/* burner_im.c
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2022
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// burner
// -----------------------------------------------------------------------------

static void im_burner_init(void *state, struct chunk *chunk, im_id id)
{
    (void) chunk;

    struct im_burner *burner = state;
    burner->id = id;
}


static void im_burner_reset(struct im_burner *burner, struct chunk *chunk)
{
    chunk_ports_reset(chunk, burner->id);
    legion_zero_from(burner, op);
}


im_energy im_burner_energy(enum item item)
{
    if (item_is_elem(item)) return item;


    const struct tape_info *info = tapes_info(item);
    if (!info) return 0;

    im_energy output = 0;
    for (enum item i = 0; i < array_len(info->elems); ++i)
        if (info->elems[i]) output += i;

    return output;
}

im_work im_burner_work_cap(enum item item)
{
    size_t sum = 1;
    if (!item_is_elem(item)) {
        const struct tape_info *info = tapes_info(item);
        if (!info) return 0;

        for (enum item i = 0; i < array_len(info->elems); ++i)
            if (info->elems[i]) sum += info->elems[i];
    }

    return legion_max(1UL, u64_log2(sum));
}

// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_burner_step_in(struct im_burner *burner, struct chunk *chunk)
{
    if (!burner->waiting) {
        chunk_ports_request(chunk, burner->id, burner->item);
        burner->waiting = true;
        return;
    }

    enum item ret = chunk_ports_consume(chunk, burner->id);
    if (!ret) return;
    assert(ret == burner->item);

    burner->op = im_burner_work;
    burner->work.left = burner->work.cap;
    burner->waiting = false;
}

static void im_burner_step_work(struct im_burner *burner, struct chunk *chunk)
{
    energy_produce_burner(chunk_energy(chunk), burner->output);

    burner->work.left--;
    if (burner->work.left) return;

    burner->op = im_burner_in;
    if (burner->loops != im_loops_inf) burner->loops--;
    if (!burner->loops) im_burner_reset(burner, chunk);
}

static void im_burner_step(void *state, struct chunk *chunk)
{
    struct im_burner *burner = state;
    if (!burner->item) return;

    switch (burner->op) {
    case im_burner_nil:  { return; }
    case im_burner_in:   { return im_burner_step_in(burner, chunk); }
    case im_burner_work: { return im_burner_step_work(burner, chunk); }
    default: { assert(false); }
    }
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_burner_io_state(
        struct im_burner *burner, struct chunk *chunk, im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, burner->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case io_item: { value = burner->item; break; }
    case io_loop: { value = burner->loops; break; }
    case io_work: { value = burner->work.cap; break; }
    case io_output: { value = burner->output; break; }
    default: { chunk_log(chunk, burner->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, burner->id, src, &value, 1);
}

static void im_burner_io_item(
        struct im_burner *burner, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, burner->id, io_item, len, 1)) return;

    enum item item = args[0];

    if (!item_validate(args[0]))
        return chunk_log(chunk, burner->id, io_item, ioe_a0_invalid);

    if (!im_check_known(chunk, burner->id, io_item, item)) return;

    im_burner_reset(burner, chunk);
    burner->op = im_burner_in;
    burner->item = item;
    burner->loops = im_loops_io(len > 1 ? args[1] : im_loops_inf);
    burner->output = im_burner_energy(item);
    burner->work.cap = im_burner_work_cap(item);
}

static void im_burner_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_burner *burner = state;

    switch(io)
    {
    case io_ping: { chunk_io(chunk, io_pong, burner->id, src, NULL, 0); return; }
    case io_state: { im_burner_io_state(burner, chunk, src, args, len); return; }

    case io_item: { im_burner_io_item(burner, chunk, args, len); return; }
    case io_reset: { im_burner_reset(burner, chunk); return; }

    default: { return; }
    }
}

static const struct io_cmd im_burner_io_list[] =
{
    { io_ping,  0, {} },
    { io_state, 1, { { "state", true } }},
    { io_reset, 0, {} },
    { io_item,  2, { { "item", true },
                     { "loops", false } }},
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_burner_flow(const void *state, struct flow *flow)
{
    const struct im_burner *burner = state;
    if (!burner->item) return false;

    enum item rank_item = burner->item;
    if (rank_item == item_elem_o) rank_item = item_elem_m;

    *flow = (struct flow) {
        .id = burner->id,
        .loops = burner->loops,
        .target = burner->item,
        .state = tape_input,
        .item = burner->item,
        .rank = tapes_info(rank_item)->rank + 1,
    };
    return true;
}
