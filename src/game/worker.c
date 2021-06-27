/* worker.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/atoms.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/active.h"


// -----------------------------------------------------------------------------
// worker
// -----------------------------------------------------------------------------

static void worker_init(void *state, id_t id, struct chunk *chunk)
{
    struct worker *worker = state;
    (void) chunk;

    *worker = (struct worker) { .id = id };
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void worker_step(void *state, struct chunk *chunk)
{
    struct worker *worker = state;

    switch (worker->state)
    {

    case worker_idle: {
        if (chunk_ports_pair(chunk, &worker->item, &worker->src, &worker->dst))
            worker->state = worker_paired;
        return;
    }

    case worker_paired: {
        item_t item = chunk_ports_take(chunk, worker->src);
        assert(item == worker->item);
        worker->state = worker_loaded;
        return;
    }

    case worker_loaded: {
        chunk_ports_give(chunk, worker->dst, worker->item);
        worker->state = worker_idle;
        return;
    }

    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void worker_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct worker *worker = state;
    (void) src, (void) len, (void) args;

    if (io == IO_PING) { chunk_io(chunk, IO_PONG, worker->id, src, 0, NULL); return; }
    return;
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct item_config *worker_config(void)
{
    static const word_t io_list[] = { IO_PING };

    static const struct item_config config = {
        .size = sizeof(struct worker),
        .init = worker_init,
        .step = worker_step,
        .io = worker_io,
        .io_list = io_list,
        .io_list_len = array_len(io_list),
    };
    return &config;
}
