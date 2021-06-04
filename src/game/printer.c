/* printer.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/atoms.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/config.h"


// -----------------------------------------------------------------------------
// printer
// -----------------------------------------------------------------------------

static const uint16_t printer_loop_inf = UINT16_MAX;

struct legion_packed printer
{
    id_t id;
    uint16_t loops;
    uint8_t blocked; // bool would take 4 bits.
    prog_it_t index;
    const struct prog *prog;
};

static_assert(sizeof(struct printer) == 16);

static void printer_init(void *state, id_t id, struct chunk *chunk)
{
    struct printer *printer = state;
    (void) chunk;

    *printer = (struct printer) { .id = id };
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void printer_step_eof(struct printer *printer)
{
    if (printer->loops != printer_loop_inf) printer->loops--;
    if (printer->loops) printer->index = 0;
}

static void printer_step_input(struct printer *printer, struct chunk *chunk, item_t item)
{
    if (!printer->blocked) {
        chunk_io_request(chunk, printer->id, item);
        printer->blocked = true;
        return;
    }

    item_t consumed = chunk_io_consume(chunk, printer->id);
    if (consumed == ITEM_NIL) return;

    assert(consumed == item);
    printer->blocked = false;
    printer->index++;
}

static void printer_step_output(struct printer *printer, struct chunk *chunk, item_t item)
{
    printer->blocked = !chunk_io_produce(chunk, printer->id, item);
    if (!printer->blocked) printer->index++;
}

static void printer_step(void *state, struct chunk *chunk)
{
    struct printer *printer = state;
    if (!printer->prog) return;

    struct prog_ret ret = prog_at(printer->prog, printer->index);
    switch (ret.state) {
    case prog_eof: { printer_step_eof(printer); return; }
    case prog_input: { printer_step_input(printer, chunk, ret.item); return; }
    case prog_output: { printer_step_output(printer, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

static void printer_cmd_reset(struct printer *printer, struct chunk *chunk)
{
    chunk_io_reset(chunk, printer->id);
    *printer = (struct printer) {
        .id = printer->id,
        .blocked = false,
        .loops = 0,
        .prog = NULL,
        .index = 0,
    };
}

static void printer_cmd_prog(
        struct printer *printer, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    uint32_t id, loops;
    vm_unpack(args[0], &id, &loops);

    if (!loops) loops = printer_loop_inf;
    if (id != (prog_id_t) id) return;

    const struct prog *prog = prog_fetch(id);
    if (!prog || prog_host(prog) != ITEM_PRINTER) return;

    printer_cmd_reset(printer, chunk);
    printer->loops = loops;
    printer->prog = prog;
}

static void printer_cmd(
        void *state, struct chunk *chunk,
        enum atom_io cmd, id_t src, size_t len, const word_t *args)
{
    struct printer *printer = state;
    (void) src;

    if (cmd == IO_PING) { chunk_cmd(chunk, IO_PONG, printer->id, src, 0, NULL); return; }
    if (cmd == IO_PROG) { printer_cmd_prog(printer, chunk, len, args); return; }
    if (cmd == IO_RESET) { printer_cmd_reset(printer, chunk); return; }
    return;
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct item_config *printer_config(void)
{
    static const struct item_config config = {
        .size = sizeof(struct printer),
        .init = printer_init,
        .step = printer_step,
        .cmd = printer_cmd,
    };
    return &config;
}
