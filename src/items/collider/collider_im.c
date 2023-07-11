/* collider_im.c
   Rémi Attab (remi.attab@gmail.com), 27 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "db/io.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// collider
// -----------------------------------------------------------------------------

uint8_t im_collider_rate(uint8_t size)
{
    const uint64_t k = im_collider_grow_max / u64_log2(im_collider_grow_max);
    return k * (log(size + 1) / log(2));
}

static void im_collider_init(void *state, struct chunk *chunk, im_id id)
{
    struct im_collider *collider = state;
    (void) chunk;

    collider->id = id;
    collider->size = 1;
    collider->rate = im_collider_rate(collider->size);
    collider->rng = rng_make(id);
}

static void im_collider_load(void *state, struct chunk *chunk)
{
    struct im_collider *collider = state;
    (void) chunk;

    enum item id = tape_packed_id(collider->tape);
    if (!id) return;

    const struct tape *tape = tapes_get(id);
    assert(tape);
    collider->tape = tape_packed_ptr_update(collider->tape, tape);
}

static void im_collider_reset(struct im_collider *collider, struct chunk *chunk)
{
    chunk_ports_reset(chunk, collider->id);
    legion_zero_from(collider, op);
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_collider_step_grow(
        struct im_collider *collider, struct chunk *chunk)
{
    if (!collider->waiting) {
        chunk_ports_request(chunk, collider->id, im_collider_grow_item);
        collider->waiting = true;
        return;
    }

    if (!chunk_ports_consume(chunk, collider->id)) return;
    collider->waiting = false;

    collider->size++;
    collider->rate = im_collider_rate(collider->size);

    collider->loops--;
    if (!collider->loops) im_collider_reset(collider, chunk);
}

static void im_collider_step_in(
        struct im_collider *collider,
        struct chunk *chunk,
        const struct tape *tape)
{
    struct tape_ret ret = tape_at(tape, tape_packed_it(collider->tape));
    assert(ret.state == tape_input);

    if (!collider->waiting) {
        chunk_ports_request(chunk, collider->id, ret.item);
        collider->waiting = true;
        return;
    }

    enum item consumed = chunk_ports_consume(chunk, collider->id);
    if (!consumed) return;
    assert(consumed == ret.item);

    collider->waiting = false;
    collider->tape = tape_packed_it_inc(collider->tape);
}

static void im_collider_step_work(
        struct im_collider *collider,
        struct chunk *chunk,
        const struct tape *tape)
{
    if (!energy_consume(chunk_energy(chunk), tape_energy(tape))) return;

    collider->tape = tape_packed_it_inc(collider->tape);

    if (tape_at(tape, tape_packed_it(collider->tape)).state == tape_output) {
        collider->out.item = 0;
        collider->out.it = 0;
        collider->out.len = tape_len(tape) / 2;
    }
}

static void im_collider_step_out(
        struct im_collider *collider, struct chunk *chunk)
{
    if (!collider->waiting) {
        uint8_t sample = rng_uni(&collider->rng, 1, im_collider_grow_max);
        collider->out.item = sample < collider->rate ?
            tape_packed_id(collider->tape) : im_collider_junk_item;

        chunk_ports_produce(chunk, collider->id, collider->out.item);
        collider->waiting = true;
        return;
    }

    if (!chunk_ports_consumed(chunk, collider->id)) return;
    collider->waiting = false;

    collider->out.it++;
    if (collider->out.it < collider->out.len) return;

    collider->tape = tape_packed_it_zero(collider->tape);

    if (collider->loops != im_loops_inf) collider->loops--;
    if (collider->loops) return;

    im_collider_reset(collider, chunk);
}

static void im_collider_step(void *state, struct chunk *chunk)
{
    struct im_collider *collider = state;

    switch (collider->op)
    {
    case im_collider_nil: { return; }
    case im_collider_grow: { im_collider_step_grow(collider, chunk); return; }

    case im_collider_tape: {
        const struct tape *tape = tape_packed_ptr(collider->tape);
        struct tape_ret ret = tape_at(tape, tape_packed_it(collider->tape));

        switch (ret.state)
        {
        case tape_input: { im_collider_step_in(collider, chunk, tape); return; }
        case tape_work: { im_collider_step_work(collider, chunk, tape); return; }
        case tape_output: { im_collider_step_out(collider, chunk); return; }
        case tape_eof:
        default: { assert(false); }
        }

        return;
    }

    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_collider_io_state(
        struct im_collider *collider, struct chunk *chunk, im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, collider->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0])
    {
    case io_size: { value = collider->size; break; }
    case io_rate: { value = collider->rate; break; }

    case io_tape: { value = tape_packed_id(collider->tape); break; }
    case io_loop: { value = collider->loops; break; }

    case io_work: {
        value = collider->tape ? tape_work_cap(tape_packed_ptr(collider->tape)) : 0;
        break;
    }

    default: { chunk_log(chunk, collider->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, collider->id, src, &value, 1);
}

static void im_collider_io_grow(
        struct im_collider *collider, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, collider->id, io_tape, len, 1)) return;

    uint8_t loops = legion_min(args[0], im_collider_grow_max - collider->size);
    if (!loops) return chunk_log(chunk, collider->id, io_grow, ioe_out_of_space);

    im_collider_reset(collider, chunk);
    collider->op = im_collider_grow;
    collider->loops = loops;
}

static void im_collider_io_tape(
        struct im_collider *collider, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, collider->id, io_tape, len, 1)) return;

    enum item item = args[0];
    if (!item_validate(args[0]))
        return chunk_log(chunk, collider->id, io_tape, ioe_a0_invalid);

    if (!im_check_known(chunk, collider->id, io_tape, item)) return;

    const struct tape *tape = tapes_get(item);
    if (!tape || tape_host(tape) != im_id_item(collider->id))
        return chunk_log(chunk, collider->id, io_tape, ioe_a0_invalid);

    im_collider_reset(collider, chunk);
    collider->op = im_collider_tape;
    collider->tape = tape_pack(item, 0, tape);
    collider->loops = im_loops_io(len > 1 ? args[1] : im_loops_inf);
}

static void im_collider_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_collider *collider = state;

    switch(io)
    {
    case io_ping: { chunk_io(chunk, io_pong, collider->id, src, NULL, 0); return; }
    case io_state: { im_collider_io_state(collider, chunk, src, args, len); return; }

    case io_grow: { im_collider_io_grow(collider, chunk, args, len); return; }
    case io_tape: { im_collider_io_tape(collider, chunk, args, len); return; }
    case io_reset: { im_collider_reset(collider, chunk); return; }

    default: { return; }
    }
}

static const struct io_cmd im_collider_io_list[] =
{
    { io_ping,  0, {} },
    { io_state, 1, { { "state", true } }},
    { io_reset, 0, {} },

    { io_grow, 0, {} },
    { io_tape, 2, { { "tape", true },
                    { "loops", false } }},
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_collider_flow(const void *state, struct flow *flow)
{
    const struct im_collider *collider = state;
    if (!collider->op) return false;

    if (collider->op == im_collider_grow) {
        *flow = (struct flow) {
            .id = collider->id,
            .loops = collider->loops,
            .target = item_collider,
            .state = tape_input,
            .item = im_collider_grow_item,
            .rank = tapes_info(im_collider_grow_item)->rank,
        };
        return true;
    }

    enum item target = tape_packed_id(collider->tape);
    const struct tape *tape = tapes_get(target);

    tape_it it = tape_packed_it(collider->tape);
    struct tape_ret ret = tape_at(tape, it);

    *flow = (struct flow) {
        .id = collider->id,
        .loops = collider->loops,
        .target = target,
        .rank = tapes_info(target)->rank,
    };

    switch (ret.state)
    {

    case tape_input:
    case tape_work: {
        flow->state = ret.state;
        flow->item = ret.item;
        flow->tape_it = it;
        flow->tape_len = tape_len(tape);
        return true;
    }

    case tape_output: {
        flow->state = tape_output;
        flow->item = collider->out.item;
        flow->tape_it = collider->out.it;
        flow->tape_len = collider->out.len;
        return true;
    }
    default: { assert(false); }
    }
}
