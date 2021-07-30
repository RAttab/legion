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

    enum item id = prog_packed_id(printer->prog);
    if (!id) return;

    const struct prog *prog = prog_fetch(id);
    assert(prog);
    printer->prog = prog_packed_ptr_update(printer->prog, prog);
}

static void printer_reset(struct printer *printer, struct chunk *chunk)
{
    chunk_ports_reset(chunk, printer->id);
    printer->waiting = false;
    printer->loops = 0;
    printer->prog = 0;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void printer_step_eof(struct printer *printer, struct chunk *chunk)
{
    if (printer->loops != loops_inf) printer->loops--;

    if (!printer->loops) printer_reset(printer, chunk);
    else printer->prog = prog_packed_it_zero(printer->prog);
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

    printer->prog = prog_packed_it_inc(printer->prog);
    printer->waiting = false;
}

static void printer_step_output(
        struct printer *printer, struct chunk *chunk, enum item item)
{
    if (!chunk_ports_produce(chunk, printer->id, item)) {
        printer->waiting = true;
        return;
    }

    printer->prog = prog_packed_it_inc(printer->prog);
    printer->waiting = false;
}

static void printer_step(void *state, struct chunk *chunk)
{
    struct printer *printer = state;

    const struct prog *prog = prog_packed_ptr(printer->prog);
    if (!prog) return;

    struct prog_ret ret = prog_at(prog, prog_packed_it(printer->prog));
    switch (ret.state) {
    case prog_eof: { printer_step_eof(printer, chunk); return; }
    case prog_input: { printer_step_input(printer, chunk, ret.item); return; }
    case prog_output: { printer_step_output(printer, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void printer_io_status(struct printer *printer, struct chunk *chunk, id_t src)
{
    word_t value = vm_pack(printer->loops, prog_packed_id(printer->prog));
    chunk_io(chunk, IO_STATE, printer->id, src, 1, &value);
}

static void printer_io_prog(
        struct printer *printer, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    word_t prog_id = args[0];
    if (prog_id != (enum item) prog_id) return;

    const struct prog *prog = prog_fetch(prog_id);
    if (!prog || prog_host(prog) != id_item(printer->id)) return;

    printer_reset(printer, chunk);
    printer->prog = prog_pack(prog_id, 0, prog);
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
    case IO_PROG: { printer_io_prog(printer, chunk, len, args); return; }
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
    static const word_t io_list[] = { IO_PING, IO_STATUS, IO_PROG, IO_RESET };

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
