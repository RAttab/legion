/* legion_im.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/chunk.h"
#include "items/io.h"


// -----------------------------------------------------------------------------
// legion
// -----------------------------------------------------------------------------

static void im_legion_init(void *state, struct chunk *chunk, id_t id)
{
    struct im_legion *legion = state;
    (void) chunk;

    legion->id = id;
}

const enum item *im_legion_cargo(enum item type)
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

static void im_legion_make(
        void *state, struct chunk *chunk, id_t id, const word_t *data, size_t len)
{
    (void) state;

    word_t src = len >= 1 ? data[0] : 0;
    word_t mod = len >= 2 ? data[1] : 0;

    for (const enum item *it = im_legion_cargo(id_item(id)); *it; ++it) {

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

static void im_legion_io_status(
        struct im_legion *legion, struct chunk *chunk, id_t src)
{
    word_t value = legion->mod;
    chunk_io(chunk, IO_STATE, legion->id, src, &value, 1);
}

static void im_legion_io_reset(struct im_legion *legion, struct chunk *chunk)
{
    chunk_ports_reset(chunk, legion->id);
    legion->mod = 0;
}

static void im_legion_io_mod(
        struct im_legion *legion, const word_t *args, size_t len)
{
    if (len < 1) return;

    mod_t id = args[0];
    if (id != args[0]) return;
    legion->mod = id;
}

static void im_legion_io_launch(
        struct im_legion *legion, struct chunk *chunk,
        const word_t *args, size_t len)
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

static void im_legion_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_legion *legion = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, legion->id, src, NULL, 0); return; }
    case IO_STATUS: { im_legion_io_status(legion, chunk, src); return; }

    case IO_MOD: { im_legion_io_mod(legion, args, len); return; }
    case IO_RESET: { im_legion_io_reset(legion, chunk); return; }

    case IO_LAUNCH: { im_legion_io_launch(legion, chunk, args, len); return; }

    default: { return; }
    }
}

static const word_t im_legion_io_list[] =
{
    IO_PING,
    IO_STATUS,

    IO_MOD,
    IO_RESET,

    IO_LAUNCH,
};
