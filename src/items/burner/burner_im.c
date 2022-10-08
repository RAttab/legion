/* burner_im.c
   Rémi Attab (remi.attab@gmail.com), 01 Aug 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "items/io.h"
#include "items/item.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// burner
// -----------------------------------------------------------------------------

static void im_burner_init(void *state, struct chunk *chunk, im_id id)
{
    (void) chunk;

    struct im_burner *burner = state;
    burner->id = id;
}


static void im_burner_reset(struct im_burner *burner, struct chunk *chunk)
{
    chunk_ports_reset(chunk, burner->id);
    legion_zero_from(burner, op);
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_burner_step_in(struct im_burner *burner, struct chunk *chunk)
{
    if (!burner->waiting) {
        chunk_ports_request(chunk, burner->id, burner->item);
        burner->waiting = true;
        return;
    }

    enum item ret = chunk_ports_consume(chunk, burner->id);
    if (!ret) return;
    assert(ret == burner->item);

    burner->op = im_burner_work;
    burner->work.left = burner->work.cap;
    burner->waiting = false;
}

static void im_burner_step_work(struct im_burner *burner, struct chunk *chunk)
{
    energy_produce(chunk_energy(chunk), burner->output);

    burner->work.left--;
    if (burner->work.left) return;

    burner->op = im_burner_in;
    if (burner->loops != im_loops_inf) burner->loops--;
    if (!burner->loops) im_burner_reset(burner, chunk);
}

static void im_burner_step(void *state, struct chunk *chunk)
{
    struct im_burner *burner = state;
    if (!burner->item) return;

    switch (burner->op) {
    case im_burner_nil:  { return; }
    case im_burner_in:   { return im_burner_step_in(burner, chunk); }
    case im_burner_work: { return im_burner_step_work(burner, chunk); }
    default: { assert(false); }
    }
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_burner_io_state(
        struct im_burner *burner, struct chunk *chunk, im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, burner->id, IO_STATE, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case IO_ITEM: { value = burner->item; break; }
    case IO_LOOP: { value = burner->loops; break; }
    case IO_WORK: { value = burner->work.cap; break; }
    case IO_OUTPUT: { value = burner->output; break; }
    default: { chunk_log(chunk, burner->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, burner->id, src, &value, 1);
}

static void im_burner_io_item(
        struct im_burner *burner, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, burner->id, IO_ITEM, len, 1)) return;

    enum item item = args[0];

    if (!item_validate(args[0]))
        return chunk_log(chunk, burner->id, IO_ITEM, IOE_A0_INVALID);

    if (!im_check_known(chunk, burner->id, IO_ITEM, item)) return;

    im_burner_reset(burner, chunk);
    burner->op = im_burner_in;
    burner->item = item;
    burner->loops = im_loops_io(len > 1 ? args[1] : im_loops_inf);

    size_t sum = 1;
    if (item_is_elem(item)) burner->output = item;
    else {
        const struct tape_info *info = tapes_info(item);
        for (enum item i = 0; i < array_len(info->elems); ++i) {
            if (!info->elems[i]) continue;
            sum += info->elems[i];
            burner->output += i;
        }
    }

    burner->work.cap = legion_max(1UL, u64_log2(sum));
}

static void im_burner_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_burner *burner = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, burner->id, src, NULL, 0); return; }
    case IO_STATE: { im_burner_io_state(burner, chunk, src, args, len); return; }

    case IO_ITEM: { im_burner_io_item(burner, chunk, args, len); return; }
    case IO_RESET: { im_burner_reset(burner, chunk); return; }

    default: { return; }
    }
}

static const struct io_cmd im_burner_io_list[] =
{
    { IO_PING,  0, {} },
    { IO_STATE, 1, { { "state", true } }},
    { IO_RESET, 0, {} },
    { IO_ITEM,  1, { { "item", true } }},
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_burner_flow(const void *state, struct flow *flow)
{
    const struct im_burner *burner = state;
    if (!burner->item) return false;

    enum item rank_item = burner->item;
    if (rank_item == ITEM_ELEM_O) rank_item = ITEM_ELEM_M;

    *flow = (struct flow) {
        .id = burner->id,
        .loops = burner->loops,
        .target = burner->item,
        .state = tape_input,
        .item = burner->item,
        .rank = tapes_info(rank_item)->rank + 1,
    };
    return true;
}
