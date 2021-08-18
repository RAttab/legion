/* legion.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/io.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/active.h"


// -----------------------------------------------------------------------------
// legion
// -----------------------------------------------------------------------------

static void legion_init(void *state, id_t id, struct chunk *chunk)
{
    struct legion *legion = state;
    (void) chunk;

    legion->id = id;
}

const enum item *legion_cargo(enum item type)
{
    switch (type)
    {
    case ITEM_LEGION_1: {
        static const enum item cargo[] = {
            ITEM_WORKER,
            ITEM_WORKER,
            ITEM_DEPLOY,
            ITEM_EXTRACT_1,
            ITEM_EXTRACT_1,
            ITEM_PRINTER_1,
            ITEM_PRINTER_1,
            ITEM_ASSEMBLY_1,
            ITEM_ASSEMBLY_1,
            ITEM_SCANNER_1,
            ITEM_DB_1,
            ITEM_BRAIN_1,
            ITEM_NIL,
        };
        return cargo;
    }

    default: { assert(false); }
    };
}

static void legion_make(
        void *state, id_t id, struct chunk *chunk, const word_t *data, size_t len)
{
    (void) state;

    word_t src = len >= 1 ? data[0] : 0;
    word_t mod = len >= 2 ? data[1] : 0;

    for (const enum item *it = legion_cargo(id_item(id)); *it; ++it) {

        switch (*it) {
        case ITEM_BRAIN_1...ITEM_BRAIN_3: {
            chunk_create_from(chunk, *it, &mod, src ? 1 : 0);
            break;
        }
        case ITEM_DB_1...ITEM_DB_3: {
            chunk_create_from(chunk, *it, &src, mod ? 1 : 0);
            break;
        }
        default: { chunk_create(chunk, *it); }
        }
    }

    chunk_delete(chunk, id);
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void legion_io_status(struct legion *legion, struct chunk *chunk, id_t src)
{
    word_t value = legion->mod;
    chunk_io(chunk, IO_STATE, legion->id, src, 1, &value);
}

static void legion_io_reset(struct legion *legion, struct chunk *chunk)
{
    chunk_ports_reset(chunk, legion->id);
    legion->mod = 0;
}

static void legion_io_mod(struct legion *legion, size_t len, const word_t *args)
{
    if (len < 1) return;

    mod_t id = args[0];
    if (id != args[0]) return;
    legion->mod = id;
}

static void legion_io_launch(
        struct legion *legion, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    struct coord dst = id_to_coord(args[0]);
    if (coord_is_nil(dst)) return;

    const word_t data[] = {
        coord_to_id(chunk_star(chunk)->coord),
        legion->mod,
    };

    chunk_lanes_launch(chunk, id_item(legion->id), dst, data, array_len(data));
    chunk_delete(chunk, legion->id);
}

static void legion_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct legion *legion = state;

    switch(io) {
    case IO_PING: { chunk_io(chunk, IO_PONG, legion->id, src, 0, NULL); return; }
    case IO_STATUS: { legion_io_status(legion, chunk, src); return; }
    case IO_MOD: { legion_io_mod(legion, len, args); return; }
    case IO_LAUNCH: { legion_io_launch(legion, chunk, len, args); return; }
    case IO_RESET: { legion_io_reset(legion, chunk); return; }
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct active_config *legion_config(enum item item)
{
    (void) item;
    static const word_t io_list[] = {
        IO_PING, IO_STATUS, IO_MOD, IO_LAUNCH, IO_RESET,
    };

    static const struct active_config config = {
        .size = sizeof(struct legion),
        .travel = 100,
        .init = legion_init,
        .make = legion_make,
        .io = legion_io,
        .io_list = io_list,
        .io_list_len = array_len(io_list),
    };
    return &config;
}
