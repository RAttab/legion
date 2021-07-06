/* legion.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/atoms.h"
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
    legion->dst = legion_waiting;

    // need to determine if it was just created or if it just arrived on a new
    // star. Lanes doesn't really allow to carry state at the moment so...
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void legion_step(void *state, struct chunk *chunk)
{
    struct legion *legion = state;

    assert(false);
    // if in new star system, create the various items.
    // delete yourself from chunk
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void legion_io_reset(struct legion *legion, struct chunk *chunk)
{
    chunk_ports_reset(chunk, progable->id);
    legion->mod = 0;
    legion->dst = legion_waiting;
}

static void legion_io_mod(
        struct legion *legion, struct chunk *chunk, size_t len, const word_t *args)
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

    // launch into the lanes but not as a logistic... need special logic here.
    // also requires that chunks removes this item from the source star.
}

static void legion_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct legion *legion = state;

    switch(io) {
    case IO_PING: { chunk_io(chunk, IO_PONG, legion->id, src, 0, NULL); return; }
    case IO_MOD: { legion_io_mod(legion, chunk, len, args); return; }
    case IO_LAUNCH: { legion_io_launch(legion, chunk, len, args); return; }
    case IO_RESET: { legion_io_reset(legion); return; }
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct item_config *legion_config(enum item item)
{
    (void) item;
    static const word_t io_list[] = { IO_PING, IO_ITEM, IO_RESET };

    static const struct item_config config = {
        .size = sizeof(struct legion),
        .init = legion_init,
        .step = legion_step,
        .io = legion_io,
        .io_list = io_list,
        .io_list_len = array_len(io_list),
    };
    return &config;
}
