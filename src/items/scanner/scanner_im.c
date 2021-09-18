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

static const word_t im_scanner_empty = -1;

static void im_scanner_init(void *state, struct chunk *chunk, id_t id)
{
    struct im_scanner *scanner = state;
    (void) chunk;

    scanner->id = id;
    scanner->result = im_scanner_empty;
}

static uint64_t im_scanner_div(enum item item)
{
    switch (item)
    {
    case ITEM_SCANNER: { return 1000; }
    default: { abort(); }
    }
}

static void im_scanner_reset(struct im_scanner *scanner)
{
    scanner->state = im_scanner_idle;
    scanner->result = im_scanner_empty;
    scanner->work.left = 0;
    scanner->work.cap = 0;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_scanner_step(void *state, struct chunk *chunk)
{
    struct im_scanner *scanner = state;

    if (scanner->state == im_scanner_idle) return;
    if (scanner->result != im_scanner_empty) return;
    if (scanner->work.left) { scanner->work.left--; return; }

    struct world *world = chunk_world(chunk);

    if (scanner->state == im_scanner_wide)
        scanner->result = coord_to_u64(world_scan_next(world, &scanner->type.wide));
    else {
        ssize_t ret = world_scan(
                world, scanner->type.target.coord, scanner->type.target.item);
        scanner->result = ret < 0 ? 0 : ret;
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_scanner_io_status(
        struct im_scanner *scanner, struct chunk *chunk, id_t src)
{
    word_t target = 0;

    switch (scanner->state) {
    case im_scanner_idle: { target = 0; break; }
    case im_scanner_wide: { target = coord_to_u64(scanner->type.wide.coord); break; }
    case im_scanner_target: { target = coord_to_u64(scanner->type.target.coord); break; }
    default: { assert(false); }
    }

    chunk_io(chunk, IO_STATE, scanner->id, src, &target, 1);
}

static void im_scanner_io_scan(
        struct im_scanner *scanner, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, scanner->id, IO_SCAN, len, 1)) return;

    struct coord coord = coord_from_u64(args[0]);
    if (!coord_validate(args[0]))
        return chunk_log(chunk, scanner->id, IO_SCAN, IOE_A0_INVALID);

    enum item item = len >= 2 ? args[1] : ITEM_NIL;
    if (len >= 2 && !item_validate(args[1]))
        return chunk_log(chunk, scanner->id, IO_SCAN, IOE_A1_INVALID);

    if (!item) {
        coord = coord_sector(coord);
        coord = make_coord(
                coord.x + coord_sector_size/2,
                coord.y + coord_sector_size/2);

        scanner->state = im_scanner_wide;
        scanner->type.wide = world_scan_it(chunk_world(chunk), coord);
    }
    else {
        scanner->state = im_scanner_target;
        scanner->type.target.item = item;
        scanner->type.target.coord = coord;
    }

    uint64_t delta = coord_dist(chunk_star(chunk)->coord, coord);
    delta /= im_scanner_div(id_item(scanner->id));

    if (delta >= UINT8_MAX) { im_scanner_reset(scanner); return; }
    scanner->work.cap = scanner->work.left = delta;
    scanner->result = im_scanner_empty;
}

static void im_scanner_io_scan_val(
        struct im_scanner *scanner, struct chunk *chunk, id_t src)
{
    chunk_io(chunk, IO_RETURN, scanner->id, src, &scanner->result, 1);

    switch (scanner->state)
    {

    case im_scanner_idle: { break; }

    case im_scanner_target: {
        if (scanner->result != im_scanner_empty)
            im_scanner_reset(scanner);
        break;
    }

    case im_scanner_wide: {
        if (scanner->result == im_scanner_empty) break;

        if (!scanner->result) {
            im_scanner_reset(scanner);
            break;
        }

        scanner->work.left = scanner->work.cap;
        scanner->result = im_scanner_empty;
        break;
    }

    default: { assert(false); }
    }
}

static void im_scanner_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_scanner *scanner = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, scanner->id, src, NULL, 0); return; }
    case IO_STATUS: { im_scanner_io_status(scanner, chunk, src); return; }

    case IO_SCAN: { im_scanner_io_scan(scanner, chunk, args, len); return; }
    case IO_SCAN_VAL: { im_scanner_io_scan_val(scanner, chunk, src); return; }
    case IO_RESET: { im_scanner_reset(scanner); return; }

    default: { return; }
    }
}

static const word_t im_scanner_io_list[] =
{
    IO_PING,
    IO_STATUS,

    IO_SCAN,
    IO_SCAN_VAL,
    IO_RESET,
};
