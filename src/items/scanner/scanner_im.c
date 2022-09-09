/* scanner_im.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "items/io.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// scanner
// -----------------------------------------------------------------------------

static const vm_word im_scanner_empty = -1;
static const uint64_t im_scanner_div = 1000;


static void im_scanner_init(void *state, struct chunk *chunk, id id)
{
    struct im_scanner *scanner = state;
    (void) chunk;

    scanner->id = id;
    scanner->result = im_scanner_empty;
}

static void im_scanner_reset(struct im_scanner *scanner)
{
    scanner->it.coord = coord_nil();
    scanner->result = im_scanner_empty;
    scanner->work.left = 0;
    scanner->work.cap = 0;
}

static uint8_t im_scanner_work(struct im_scanner *scanner, struct chunk *chunk)
{
    struct coord origin = chunk_star(chunk)->coord;
    struct coord next = world_scan_peek(chunk_world(chunk), &scanner->it);

    uint64_t delta = coord_dist(origin, next) / im_scanner_div;
    return delta < UINT8_MAX ? delta : UINT8_MAX;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_scanner_step(void *state, struct chunk *chunk)
{
    struct im_scanner *scanner = state;

    if (coord_is_nil(scanner->it.coord)) return;
    if (scanner->result != im_scanner_empty) return;
    if (scanner->work.left) { scanner->work.left--; return; }

    struct coord coord = world_scan_next(chunk_world(chunk), &scanner->it);

    if (coord_is_nil(coord)) im_scanner_reset(scanner);
    else scanner->result = coord_to_u64(coord);
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_scanner_io_state(
        struct im_scanner *scanner, struct chunk *chunk, id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, scanner->id, IO_STATE, len, 1)) return;
    vm_word value = 0;

    switch (args[0])
    {
    case IO_TARGET: { value = coord_to_u64(coord_sector(scanner->it.coord)); break; }
    default: { chunk_log(chunk, scanner->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, scanner->id, src, &value, 1);
}

static void im_scanner_io_scan(
        struct im_scanner *scanner, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, scanner->id, IO_SCAN, len, 1)) return;

    struct coord coord = coord_from_u64(args[0]);

    struct coord origin = chunk_star(chunk)->coord;
    if (coord_is_nil(coord)) coord = origin;

    scanner->it = world_scan_it(chunk_world(chunk), coord_sector(coord));
    scanner->work.cap = scanner->work.left = im_scanner_work(scanner, chunk);
    scanner->result = im_scanner_empty;
}

static void im_scanner_io_value(
        struct im_scanner *scanner, struct chunk *chunk, id src)
{
    chunk_io(chunk, IO_RETURN, scanner->id, src, &scanner->result, 1);
    if (scanner->result == im_scanner_empty) return;

    if (!scanner->result) {
        im_scanner_reset(scanner);
        return;
    }

    scanner->work.cap = scanner->work.left = im_scanner_work(scanner, chunk);
    scanner->result = im_scanner_empty;
}

static void im_scanner_io(
        void *state, struct chunk *chunk,
        enum io io, id src,
        const vm_word *args, size_t len)
{
    struct im_scanner *scanner = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, scanner->id, src, NULL, 0); return; }
    case IO_STATE: { im_scanner_io_state(scanner, chunk, src, args, len); return; }

    case IO_SCAN: { im_scanner_io_scan(scanner, chunk, args, len); return; }
    case IO_VALUE: { im_scanner_io_value(scanner, chunk, src); return; }
    case IO_RESET: { im_scanner_reset(scanner); return; }

    default: { return; }
    }
}

static const vm_word im_scanner_io_list[] =
{
    IO_PING,
    IO_STATE,

    IO_SCAN,
    IO_VALUE,
    IO_RESET,
};
