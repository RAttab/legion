/* deploy_im.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "db/io.h"
#include "db/items.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// deploy
// -----------------------------------------------------------------------------

static void im_deploy_init(void *state, struct chunk *chunk, im_id id)
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
        chunk_log(chunk, deploy->id, io_step, ioe_out_of_space);

    deploy->waiting = false;
    if (deploy->loops != im_loops_inf) --deploy->loops;
    if (!deploy->loops) im_deploy_reset(deploy, chunk);
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_deploy_io_state(
        struct im_deploy *deploy, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, deploy->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case io_item: { value = deploy->item; break; }
    case io_loop: { value = deploy->loops; break; }
    default: { chunk_log(chunk, deploy->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, deploy->id, src, &value, 1);
}

static void im_deploy_io_item(
        struct im_deploy *deploy, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, deploy->id, io_item, len, 1)) return;

    enum item item = args[0];

    if (!item_validate(args[0]))
        return chunk_log(chunk, deploy->id, io_item, ioe_a0_invalid);

    if (!item_is_active(item) && !item_is_logistics(item))
        return chunk_log(chunk, deploy->id, io_item, ioe_a0_invalid);

    if (!im_check_known(chunk, deploy->id, io_item, item)) return;

    im_deploy_reset(deploy, chunk);
    deploy->item = item;
    deploy->loops = im_loops_io(len > 1 ? args[1] : im_loops_inf);
}

static void im_deploy_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_deploy *deploy = state;

    switch(io)
    {
    case io_ping: { chunk_io(chunk, io_pong, deploy->id, src, NULL, 0); return; }
    case io_state: { im_deploy_io_state(deploy, chunk, src, args, len); return; }

    case io_item: { im_deploy_io_item(deploy, chunk, args, len); return; }
    case io_reset: { im_deploy_reset(deploy, chunk); return; }

    default: { return; }
    }
}

static const struct io_cmd im_deploy_io_list[] =
{
    { io_ping,  0, {} },
    { io_state, 1, { { "state", true } }},
    { io_reset, 0, {} },
    { io_item,  2, { { "item", true },
                     { "loops", false } }},
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
        .state = tape_input,
        .item = deploy->item,
        .rank = tapes_info(deploy->item)->rank + 1,
    };
    return true;
}
