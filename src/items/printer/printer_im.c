/* printer_im.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/


// -----------------------------------------------------------------------------
// printer
// -----------------------------------------------------------------------------

static void im_printer_init(void *state, struct chunk *chunk, im_id id)
{
    struct im_printer *printer = state;
    (void) chunk;

    printer->id = id;
}

static void im_printer_load(void *state, struct chunk *chunk)
{
    struct im_printer *printer = state;
    (void) chunk;

    enum item id = tape_packed_id(printer->tape);
    if (!id) return;

    const struct tape *tape = tapes_get(id);
    assert(tape);
    printer->tape = tape_packed_ptr_update(printer->tape, tape);
}

static void im_printer_reset(struct im_printer *printer, struct chunk *chunk)
{
    chunk_ports_reset(chunk, printer->id);
    printer->waiting = false;
    printer->loops = 0;
    printer->tape = 0;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_printer_step_eof(struct im_printer *printer, struct chunk *chunk)
{
    if (printer->loops != im_loops_inf) printer->loops--;

    if (!printer->loops) im_printer_reset(printer, chunk);
    else printer->tape = tape_packed_it_zero(printer->tape);
}

static void im_printer_step_input(
        struct im_printer *printer, struct chunk *chunk, enum item item)
{
    if (!printer->waiting) {
        chunk_ports_request(chunk, printer->id, item);
        printer->waiting = true;
        return;
    }

    enum item consumed = chunk_ports_consume(chunk, printer->id);
    if (!consumed) return;
    assert(consumed == item);

    printer->tape = tape_packed_it_inc(printer->tape);
    printer->waiting = false;
}

static void im_printer_step_work(
        struct im_printer *printer, struct chunk *chunk, const struct tape *tape)
{
    if (energy_consume(chunk_energy(chunk), tape_energy(tape)))
        printer->tape = tape_packed_it_inc(printer->tape);
}

static void im_printer_step_output(
        struct im_printer *printer, struct chunk *chunk, enum item item)
{
    if (!printer->waiting) {
        printer->waiting = chunk_ports_produce(chunk, printer->id, item);
        assert(printer->waiting);
        return;
    }

    if (!chunk_ports_consumed(chunk, printer->id)) return;

    printer->tape = tape_packed_it_inc(printer->tape);
    printer->waiting = false;
}

static void im_printer_step(void *state, struct chunk *chunk)
{
    struct im_printer *printer = state;

    const struct tape *tape = tape_packed_ptr(printer->tape);
    if (!tape) return;

    struct tape_ret ret = tape_at(tape, tape_packed_it(printer->tape));
    switch (ret.state) {
    case tape_eof: { im_printer_step_eof(printer, chunk); return; }
    case tape_input: { im_printer_step_input(printer, chunk, ret.item); return; }
    case tape_work: { im_printer_step_work(printer, chunk, tape); return; }
    case tape_output: { im_printer_step_output(printer, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_printer_io_state(
        struct im_printer *printer, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, printer->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case io_tape: { value = tape_packed_id(printer->tape); break; }
    case io_loop: { value = printer->loops; break; }
    default: { chunk_log(chunk, printer->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, printer->id, src, &value, 1);
}

static void im_printer_io_tape(
        struct im_printer *printer, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, printer->id, io_tape, len, 1)) return;

    enum item item = args[0];
    if (!item_validate(args[0]))
        return chunk_log(chunk, printer->id, io_tape, ioe_a0_invalid);

    if (!im_check_known(chunk, printer->id, io_tape, item)) return;

    const struct tape *tape = tapes_get(item);
    if (!tape || tape_host(tape) != im_id_item(printer->id))
        return chunk_log(chunk, printer->id, io_tape, ioe_a0_invalid);

    im_printer_reset(printer, chunk);
    printer->tape = tape_pack(item, 0, tape);
    printer->loops = im_loops_io(len > 1 ? args[1] : im_loops_inf);
}

static void im_printer_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_printer *printer = state;

    switch(io)
    {
    case io_ping: { chunk_io(chunk, io_pong, printer->id, src, NULL, 0); return; }
    case io_state: { im_printer_io_state(printer, chunk, src, args, len); return; }

    case io_tape: { im_printer_io_tape(printer, chunk, args, len); return; }
    case io_reset: { im_printer_reset(printer, chunk); return; }

    default: { return; }
    }
}

static const struct io_cmd im_printer_io_list[] =
{
    { io_ping,  0, {} },
    { io_state, 1, { { "state", true } }},
    { io_reset, 0, {} },
    { io_tape,  2, { { "tape", true },
                     { "loops", false } }},
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_printer_flow(const void *state, struct flow *flow)
{
    const struct im_printer *printer = state;
    if (!printer->tape) return false;

    enum item target = tape_packed_id(printer->tape);
    const struct tape *tape = tapes_get(target);
    struct tape_ret ret = tape_at(tape, tape_packed_it(printer->tape));

    *flow = (struct flow) {
        .id = printer->id,
        .loops = printer->loops,
        .target = target,
        .state = ret.state,
        .item = ret.state ? ret.item : item_nil,
        .tape_it = tape_packed_it(printer->tape),
        .tape_len = tape_len(tape),
        .rank = tapes_info(target)->rank,
    };

    return true;
}
