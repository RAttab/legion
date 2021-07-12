/* deploy.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/atoms.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/active.h"


// -----------------------------------------------------------------------------
// deploy
// -----------------------------------------------------------------------------

static void deploy_init(void *state, id_t id, struct chunk *chunk)
{
    struct deploy *deploy = state;
    (void) chunk;

    deploy->id = id;
}


// -----------------------------------------------------------------------------
// deploy
// -----------------------------------------------------------------------------

static void deploy_step(void *state, struct chunk *chunk)
{
    struct deploy *deploy = state;
    if (!deploy->item) return;

    if (!deploy->waiting) {
        chunk_ports_request(chunk, deploy->id, deploy->item);
        deploy->waiting = true;
        return;
    }

    enum item ret = chunk_ports_consume(chunk, deploy->id);
    if (!ret) return;
    assert(ret == deploy->item);

    chunk_create(chunk, deploy->item);
    
    deploy->waiting = false;

    if (deploy->loops != loops_inf) --deploy->loops;
    if (!deploy->loops) deploy->item = 0;
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void deploy_io_reset(struct deploy *deploy, struct chunk *chunk)
{
    chunk_ports_reset(chunk, deploy->id);
    deploy->item = 0;
    deploy->loops = 0;
    deploy->waiting = false;
}

static void deploy_io_item(
        struct deploy *deploy, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    word_t item = args[0];
    if (item != (enum item) item) return;
    if (!item_is_active(item)) return;

    deploy_io_reset(deploy, chunk);
    deploy->item = item;
    deploy->loops = loops_io(len > 1 ? args[1] : loops_inf);
}

static void deploy_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct deploy *deploy = state;

    switch(io) {
    case IO_PING: { chunk_io(chunk, IO_PONG, deploy->id, src, 0, NULL); return; }
    case IO_ITEM: { deploy_io_item(deploy, chunk, len, args); return; }
    case IO_RESET: { deploy_io_reset(deploy, chunk); return; }
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct active_config *deploy_config(enum item item)
{
    (void) item;
    static const word_t io_list[] = { IO_PING, IO_ITEM, IO_RESET };

    static const struct active_config config = {
        .size = sizeof(struct deploy),
        .init = deploy_init,
        .step = deploy_step,
        .io = deploy_io,
        .io_list = io_list,
        .io_list_len = array_len(io_list),
    };
    return &config;
}
