/* deploy_im.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/io.h"
#include "items/item.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// deploy
// -----------------------------------------------------------------------------

static void im_deploy_init(void *state, struct chunk *chunk, id_t id)
{
    (void) chunk;

    struct im_deploy *deploy = state;
    deploy->id = id;
}


static void im_deploy_reset(struct im_deploy *deploy, struct chunk *chunk)
{
    chunk_ports_reset(chunk, deploy->id);
    deploy->item = 0;
    deploy->loops = 0;
    deploy->waiting = false;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_deploy_step(void *state, struct chunk *chunk)
{
    struct im_deploy *deploy = state;
    if (!deploy->item) return;

    if (!deploy->waiting) {
        chunk_ports_request(chunk, deploy->id, deploy->item);
        deploy->waiting = true;
        return;
    }

    enum item ret = chunk_ports_consume(chunk, deploy->id);
    if (!ret) return;
    assert(ret == deploy->item);

    if (!chunk_create(chunk, deploy->item))
        chunk_log(chunk, deploy->id, IO_STEP, IOE_OUT_OF_SPACE);

    deploy->waiting = false;
    if (deploy->loops != loops_inf) --deploy->loops;
    if (!deploy->loops) im_deploy_reset(deploy, chunk);
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_deploy_io_state(
        struct im_deploy *deploy, struct chunk *chunk, id_t src,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, deploy->id, IO_STATE, len, 1)) return;
    word_t value = 0;

    switch (args[0]) {
    case IO_ITEM: { value = deploy->item; break; }
    case IO_LOOP: { value = deploy->loops; break; }
    default: { chunk_log(chunk, deploy->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, deploy->id, src, &value, 1);
}

static void im_deploy_io_item(
        struct im_deploy *deploy, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, deploy->id, IO_ITEM, len, 1)) return;

    enum item item = args[0];

    if (!item_validate(args[0]))
        return chunk_log(chunk, deploy->id, IO_ITEM, IOE_A0_INVALID);

    if (!item_is_active(item) && !item_is_logistics(item))
        return chunk_log(chunk, deploy->id, IO_ITEM, IOE_A0_INVALID);

    if (!im_check_known(chunk, deploy->id, IO_ITEM, item)) return;

    im_deploy_reset(deploy, chunk);
    deploy->item = item;
    deploy->loops = loops_io(len > 1 ? args[1] : loops_inf);
}

static void im_deploy_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_deploy *deploy = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, deploy->id, src, NULL, 0); return; }
    case IO_STATE: { im_deploy_io_state(deploy, chunk, src, args, len); return; }

    case IO_ITEM: { im_deploy_io_item(deploy, chunk, args, len); return; }
    case IO_RESET: { im_deploy_reset(deploy, chunk); return; }

    default: { return; }
    }
}

static const word_t im_deploy_io_list[] =
{
    IO_PING,
    IO_STATE,

    IO_ITEM,
    IO_RESET
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_deploy_flow(const void *state, struct flow *flow)
{
    const struct im_deploy *deploy = state;
    if (!deploy->item) return false;

    *flow = (struct flow) {
        .id = deploy->id,
        .loops = deploy->loops,
        .target = deploy->item,
        .in = deploy->item,
        .rank = tapes_info(deploy->item)->rank + 1,
    };
    return true;
}
