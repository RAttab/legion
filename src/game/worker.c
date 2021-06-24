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

    if (worker->item) {
        chunk_ports_give(chunk, worker->dst, worker->item);
        return;
    }

    // we have to take when we pair otherwise we risk race conditions with other
    // workers.
    if (chunk_ports_pair(chunk, &worker->item, &worker->src, &worker->dst)) {
        item_t item = chunk_ports_take(chunk, worker->src);
        assert(item == worker->item);
    }
}


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

static void worker_cmd(
        void *state, struct chunk *chunk,
        enum atom_io cmd, id_t src, size_t len, const word_t *args)
{
    struct worker *worker = state;
    (void) src, (void) len, (void) args;

    if (cmd == IO_PING) { chunk_cmd(chunk, IO_PONG, worker->id, src, 0, NULL); return; }
    return;
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct item_config *worker_config(void)
{
    static const struct item_config config = {
        .size = sizeof(struct worker),
        .init = worker_init,
        .step = worker_step,
        .cmd = worker_cmd,
    };
    return &config;
}
