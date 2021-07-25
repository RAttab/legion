/* scanner.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/io.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/active.h"


// -----------------------------------------------------------------------------
// scanner
// -----------------------------------------------------------------------------

static void scanner_init(void *state, id_t id, struct chunk *chunk)
{
    struct scanner *scanner = state;
    (void) chunk;

    scanner->id = id;
}

static uint64_t scanner_div(enum item item)
{
    switch (item)
    {
    case ITEM_SCANNER_1: { return 1000; }
    case ITEM_SCANNER_2: { return 100000; }
    case ITEM_SCANNER_3: { return 10000000; }
    default: { abort(); }
    }
}

// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void scanner_step(void *state, struct chunk *chunk)
{
    struct scanner *scanner = state;

    if (scanner->state == scanner_idle) return;
    if (scanner->work.left) { scanner->work.left--; return; }
    if (scanner->result) return;

    if (scanner->state == scanner_wide) {
        scanner->result = coord_to_id(world_scan_next(chunk_world(chunk), &scanner->type.wide));
        if (!scanner->result) scanner->state = scanner_idle;
        else scanner->work.left = scanner->work.cap;
    }
    else {
        scanner->result = world_scan(chunk_world(chunk),
                scanner->type.target.coord, scanner->type.target.item);
        scanner->state = scanner_idle;
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void scanner_io_status(struct scanner *scanner, struct chunk *chunk, id_t src)
{
    word_t target = 0;
    if (scanner->state == scanner_wide)
        target = coord_to_id(scanner->type.wide.coord);
    else if (scanner->state == scanner_target)
        target = coord_to_id(scanner->type.target.coord);

    word_t value[] = { target, scanner->state };
    chunk_io(chunk, IO_STATE, scanner->id, src, array_len(value), value);
}

static void scanner_io_reset(struct scanner *scanner)
{
    scanner->state = scanner_idle;
    scanner->work.left = 0;
    scanner->work.cap = 0;
    scanner->result = 0;
}


static void scanner_io_scan(
        struct scanner *scanner, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;
    if (len >= 2 && !args[1]) len = 1;

    struct coord coord = id_to_coord(args[0]);

    if (len < 2) {
        coord = coord_sector(coord);
        coord = make_coord(coord.x + coord_sector_size/2, coord.y + coord_sector_size/2);

        scanner->state = scanner_wide;
        scanner->type.wide = world_scan_it(chunk_world(chunk), coord);
    }
    else {
        enum item item = args[1];
        if (item != args[1]) return;

        scanner->state = scanner_target;
        scanner->type.target.item = item;
        scanner->type.target.coord = coord;
    }

    uint64_t delta = coord_dist(chunk_star(chunk)->coord, coord);
    delta /= scanner_div(id_item(scanner->id));

    if (delta >= UINT8_MAX) { scanner_io_reset(scanner); return; }
    scanner->work.cap = scanner->work.left = delta;
    scanner->result = 0;
}

static void scanner_io_result(struct scanner *scanner, struct chunk *chunk, id_t src)
{
    chunk_io(chunk, IO_VAL, scanner->id, src, 1, &scanner->result);
    scanner->result = 0;
}

static void scanner_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct scanner *scanner = state;

    switch(io) {
    case IO_PING: { chunk_io(chunk, IO_PONG, scanner->id, src, 0, NULL); return; }
    case IO_STATUS: { scanner_io_status(scanner, chunk, src); return; }
    case IO_SCAN: { scanner_io_scan(scanner, chunk, len, args); return; }
    case IO_RESULT: { scanner_io_result(scanner, chunk, src); return; }
    case IO_RESET: { scanner_io_reset(scanner); return; }
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct active_config *scanner_config(enum item item)
{
    (void) item;
    static const word_t io_list[] = {
        IO_PING, IO_STATUS, IO_SCAN, IO_RESULT, IO_RESET
    };

    static const struct active_config config = {
        .size = sizeof(struct scanner),
        .init = scanner_init,
        .step = scanner_step,
        .io = scanner_io,
        .io_list = io_list,
        .io_list_len = array_len(io_list),
    };
    return &config;
}