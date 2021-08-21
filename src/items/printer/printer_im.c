/* printer_im.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "items/io.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// printer
// -----------------------------------------------------------------------------

static void im_printer_init(void *state, struct chunk *chunk, id_t id)
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
    if (printer->loops != loops_inf) printer->loops--;

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
    case tape_output: { im_printer_step_output(printer, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_printer_io_status(
        struct im_printer *printer, struct chunk *chunk, id_t src)
{
    word_t value = vm_pack(printer->loops, tape_packed_id(printer->tape));
    chunk_io(chunk, IO_STATE, printer->id, src, &value, 1);
}

static void im_printer_io_tape(
        struct im_printer *printer, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (len < 1) return;

    word_t tape_id = args[0];
    if (tape_id != (enum item) tape_id) return;

    const struct tape *tape = tapes_get(tape_id);
    if (!tape || tape_host(tape) != id_item(printer->id)) return;

    im_printer_reset(printer, chunk);
    printer->tape = tape_pack(tape_id, 0, tape);
    printer->loops = loops_io(len > 1 ? args[1] : loops_inf);
}

static void im_printer_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_printer *printer = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, printer->id, src, NULL, 0); return; }
    case IO_STATUS: { im_printer_io_status(printer, chunk, src); return; }

    case IO_TAPE: { im_printer_io_tape(printer, chunk, args, len); return; }
    case IO_RESET: { im_printer_reset(printer, chunk); return; }

    default: { return; }
    }
}

static const word_t im_printer_io_list[] =
{
    IO_PING,
    IO_STATUS,

    IO_TAPE,
    IO_RESET,
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_printer_flow(const void *state, struct flow *flow)
{
    const struct im_printer *printer = state;
    if (!printer->tape) return false;

    *flow = (struct flow) {
        .id = printer->id,
        .loops = printer->loops,
        .target = tape_packed_id(printer->tape),
    };

    const struct tape *tape = tape_packed_ptr(printer->tape);
    struct tape_ret ret = tape_at(tape, tape_packed_it(printer->tape));

    switch (ret.state) {
    case tape_input: { flow->in = ret.item; break; }
    case tape_output: { flow->out = ret.item; break; }
    default: { break; }
    }

    flow->tape_it = tape_packed_it(printer->tape);
    flow->tape_len = tape_len(tape);
    return true;
}