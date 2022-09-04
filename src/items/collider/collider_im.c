/* collider_im.c
   Rémi Attab (remi.attab@gmail.com), 27 Jul 2022
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "items/io.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// collider
// -----------------------------------------------------------------------------

uint8_t im_collider_rate(uint8_t size)
{
    const uint64_t k = im_collider_size_max / u64_log2(im_collider_size_max);
    return k * (log(size + 1) / log(2));
}

static void im_collider_init(void *state, struct chunk *chunk, id_t id)
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
        struct im_collider *collider, struct chunk *chunk)
{
    const struct tape *tape = tape_packed_ptr(collider->tape);
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
    if (tape_at(tape, tape_packed_it(collider->tape)).state == tape_input) return;

    collider->op = im_collider_work;
    collider->work.left = collider->work.cap = tape_work(tape);
}

static void im_collider_step_work(
        struct im_collider *collider, struct chunk *chunk)
{
    if (collider->work.left) {
        const struct tape *tape = tape_packed_ptr(collider->tape);
        if (!energy_consume(chunk_energy(chunk), tape_energy(tape))) return;

        collider->work.left--;
        if (collider->work.left) return;
    }

    const struct tape *tape = tape_packed_ptr(collider->tape);

    collider->op = im_collider_out;
    collider->out.item = 0;
    collider->out.it = 0;
    collider->out.len = tape_len(tape) / 2;
}

static void im_collider_step_out(
        struct im_collider *collider, struct chunk *chunk)
{
    if (!collider->waiting) {
        uint8_t sample = rng_uni(&collider->rng, 1, im_collider_size_max);
        collider->out.item = sample < collider->rate ?
            tape_packed_id(collider->tape) : im_collider_junk;

        chunk_ports_produce(chunk, collider->id, collider->out.item);
        collider->waiting = true;
        return;
    }

    if (!chunk_ports_consumed(chunk, collider->id)) return;
    collider->waiting = false;

    collider->out.it++;
    if (collider->out.it < collider->out.len) return;

    collider->op = im_collider_in;
    collider->tape = tape_packed_it_zero(collider->tape);

    if (collider->loops != loops_inf) collider->loops--;
    if (collider->loops) return;

    im_collider_reset(collider, chunk);
}

static void im_collider_step(void *state, struct chunk *chunk)
{
    struct im_collider *collider = state;

    switch (collider->op) {
    case im_collider_nil: { return; }
    case im_collider_grow: { im_collider_step_grow(collider, chunk); return; }
    case im_collider_in: { im_collider_step_in(collider, chunk); return; }
    case im_collider_work: { im_collider_step_work(collider, chunk); return; }
    case im_collider_out: { im_collider_step_out(collider, chunk); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_collider_io_state(
        struct im_collider *collider, struct chunk *chunk, id_t src,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, collider->id, IO_STATE, len, 1)) return;
    word_t value = 0;

    switch (args[0])
    {
    case IO_SIZE: { value = collider->size; break; }
    case IO_RATE: { value = collider->rate; break; }

    case IO_TAPE: { value = tape_packed_id(collider->tape); break; }
    case IO_LOOP: { value = collider->loops; break; }

    case IO_WORK: {
        value = collider->tape ? tape_work(tape_packed_ptr(collider->tape)) : 0;
        break;
    }

    default: { chunk_log(chunk, collider->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, collider->id, src, &value, 1);
}

static void im_collider_io_grow(
        struct im_collider *collider, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, collider->id, IO_TAPE, len, 1)) return;

    uint8_t loops = legion_min(args[0], im_collider_size_max - collider->size);
    if (!loops) return chunk_log(chunk, collider->id, IO_GROW, IOE_OUT_OF_SPACE);

    im_collider_reset(collider, chunk);
    collider->op = im_collider_grow;
    collider->loops = loops;
}

static void im_collider_io_tape(
        struct im_collider *collider, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, collider->id, IO_TAPE, len, 1)) return;

    enum item item = args[0];
    if (!item_validate(args[0]))
        return chunk_log(chunk, collider->id, IO_TAPE, IOE_A0_INVALID);

    if (!im_check_known(chunk, collider->id, IO_TAPE, item)) return;

    const struct tape *tape = tapes_get(item);
    if (!tape || tape_host(tape) != id_item(collider->id))
        return chunk_log(chunk, collider->id, IO_TAPE, IOE_A0_INVALID);

    im_collider_reset(collider, chunk);
    collider->op = im_collider_in;
    collider->tape = tape_pack(item, 0, tape);
    collider->loops = loops_io(len > 1 ? args[1] : loops_inf);
}

static void im_collider_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_collider *collider = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, collider->id, src, NULL, 0); return; }
    case IO_STATE: { im_collider_io_state(collider, chunk, src, args, len); return; }

    case IO_GROW: { im_collider_io_grow(collider, chunk, args, len); return; }
    case IO_TAPE: { im_collider_io_tape(collider, chunk, args, len); return; }
    case IO_RESET: { im_collider_reset(collider, chunk); return; }

    default: { return; }
    }
}

static const word_t im_collider_io_list[] =
{
    IO_PING,
    IO_STATE,

    IO_GROW,
    IO_TAPE,
    IO_RESET,
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_collider_flow(const void *state, struct flow *flow)
{
    const struct im_collider *collider = state;

    switch (collider->op)
    {
    case im_collider_nil:
    case im_collider_work: { return false; }

    case im_collider_grow: {
        *flow = (struct flow) {
            .id = collider->id,
            .loops = collider->loops,
            .target = ITEM_COLLIDER,
            .rank = tapes_info(im_collider_grow_item)->rank,
            .in = im_collider_grow_item,
        };
        return true;
    }

    case im_collider_in: {
        enum item target = tape_packed_id(collider->tape);
        const struct tape *tape = tapes_get(target);

        tape_it_t it = tape_packed_it(collider->tape);
        struct tape_ret ret = tape_at(tape, it);
        assert(ret.state == tape_input);

        *flow = (struct flow) {
            .id = collider->id,
            .loops = collider->loops,
            .target = target,
            .rank = tapes_info(ret.item)->rank,
            .in = ret.item,
            .tape_it = it,
            .tape_len = tape_len(tape),
        };
        return true;
    }

    case im_collider_out: {
        if (!collider->out.item) return false;
        enum item out = tape_packed_id(collider->tape);
        *flow = (struct flow) {
            .id = collider->id,
            .loops = collider->loops,
            .target = out,
            .rank = tapes_info(out)->rank,
            .out = collider->out.item,
            .tape_it = collider->out.it,
            .tape_len = collider->out.len,
        };
        return true;
    }

    default: { assert(false); }
    }
}