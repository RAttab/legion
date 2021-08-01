/* printer.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/io.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/active.h"


// -----------------------------------------------------------------------------
// printer
// -----------------------------------------------------------------------------

static void printer_init(void *state, id_t id, struct chunk *chunk)
{
    struct printer *printer = state;
    (void) chunk;

    printer->id = id;
}

static void printer_load(void *state, struct chunk *chunk)
{
    struct printer *printer = state;
    (void) chunk;

    enum item id = tape_packed_id(printer->tape);
    if (!id) return;

    const struct tape *tape = tape_fetch(id);
    assert(tape);
    printer->tape = tape_packed_ptr_update(printer->tape, tape);
}

static void printer_reset(struct printer *printer, struct chunk *chunk)
{
    chunk_ports_reset(chunk, printer->id);
    printer->waiting = false;
    printer->loops = 0;
    printer->tape = 0;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void printer_step_eof(struct printer *printer, struct chunk *chunk)
{
    if (printer->loops != loops_inf) printer->loops--;

    if (!printer->loops) printer_reset(printer, chunk);
    else printer->tape = tape_packed_it_zero(printer->tape);
}

static void printer_step_input(
        struct printer *printer, struct chunk *chunk, enum item item)
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

static void printer_step_output(
        struct printer *printer, struct chunk *chunk, enum item item)
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

static void printer_step(void *state, struct chunk *chunk)
{
    struct printer *printer = state;

    const struct tape *tape = tape_packed_ptr(printer->tape);
    if (!tape) return;

    struct tape_ret ret = tape_at(tape, tape_packed_it(printer->tape));
    switch (ret.state) {
    case tape_eof: { printer_step_eof(printer, chunk); return; }
    case tape_input: { printer_step_input(printer, chunk, ret.item); return; }
    case tape_output: { printer_step_output(printer, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void printer_io_status(struct printer *printer, struct chunk *chunk, id_t src)
{
    word_t value = vm_pack(printer->loops, tape_packed_id(printer->tape));
    chunk_io(chunk, IO_STATE, printer->id, src, 1, &value);
}

static void printer_io_tape(
        struct printer *printer, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    word_t tape_id = args[0];
    if (tape_id != (enum item) tape_id) return;

    const struct tape *tape = tape_fetch(tape_id);
    if (!tape || tape_host(tape) != id_item(printer->id)) return;

    printer_reset(printer, chunk);
    printer->tape = tape_pack(tape_id, 0, tape);
    printer->loops = loops_io(len > 1 ? args[1] : loops_inf);
}

static void printer_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct printer *printer = state;

    switch(io) {
    case IO_PING: { chunk_io(chunk, IO_PONG, printer->id, src, 0, NULL); return; }
    case IO_STATUS: { printer_io_status(printer, chunk, src); return; }
    case IO_TAPE: { printer_io_tape(printer, chunk, len, args); return; }
    case IO_RESET: { printer_reset(printer, chunk); return; }
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct active_config *printer_config(enum item item)
{
    (void) item;
    static const word_t io_list[] = { IO_PING, IO_STATUS, IO_TAPE, IO_RESET };

    static const struct active_config config = {
        .size = sizeof(struct printer),
        .init = printer_init,
        .load = printer_load,
        .step = printer_step,
        .io = printer_io,
        .io_list = io_list,
        .io_list_len = array_len(io_list),
    };
    return &config;
}
