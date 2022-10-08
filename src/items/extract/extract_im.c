/* extract_im.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/chunk.h"
#include "game/tape.h"
#include "items/io.h"
#include "items/types.h"


// -----------------------------------------------------------------------------
// extract
// -----------------------------------------------------------------------------

static void im_extract_init(void *state, struct chunk *chunk, im_id id)
{
    struct im_extract *extract = state;
    (void) chunk;

    extract->id = id;
}

static void im_extract_load(void *state, struct chunk *chunk)
{
    struct im_extract *extract = state;
    (void) chunk;

    enum item id = tape_packed_id(extract->tape);
    if (!id) return;

    const struct tape *tape = tapes_get(id);
    assert(tape);
    extract->tape = tape_packed_ptr_update(extract->tape, tape);
}

static void im_extract_reset(struct im_extract *extract, struct chunk *chunk)
{
    chunk_ports_reset(chunk, extract->id);
    extract->waiting = false;
    extract->loops = 0;
    extract->tape = 0;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_extract_step_eof(struct im_extract *extract, struct chunk *chunk)
{
    if (extract->loops != im_loops_inf) extract->loops--;

    if (!extract->loops) im_extract_reset(extract, chunk);
    else extract->tape = tape_packed_it_zero(extract->tape);
}

static void im_extract_step_input(
        struct im_extract *extract, struct chunk *chunk, enum item item)
{
    if (!extract->waiting) {
        chunk_ports_request(chunk, extract->id, item);
        extract->waiting = true;
        return;
    }

    enum item consumed = chunk_ports_consume(chunk, extract->id);
    if (!consumed) return;
    assert(consumed == item);

    extract->waiting = false;
    extract->tape = tape_packed_it_inc(extract->tape);
}

static void im_extract_step_work(
        struct im_extract *extract, struct chunk *chunk, const struct tape *tape)
{
    if (energy_consume(chunk_energy(chunk), tape_energy(tape)))
        extract->tape = tape_packed_it_inc(extract->tape);
}

static void im_extract_step_output(
        struct im_extract *extract, struct chunk *chunk, enum item item)
{
    if (!extract->waiting) {
        if (!chunk_harvest(chunk, item)) {
            chunk_log(chunk, extract->id, IO_STEP, IOE_STARVED);
            im_extract_reset(extract, chunk);
            return;
        }

        extract->waiting = chunk_ports_produce(chunk, extract->id, item);
        assert(extract->waiting);
        return;
    }

    if (!chunk_ports_consumed(chunk, extract->id)) return;

    extract->tape = tape_packed_it_inc(extract->tape);
    extract->waiting = false;
}

static void im_extract_step(void *state, struct chunk *chunk)
{
    struct im_extract *extract = state;

    const struct tape *tape = tape_packed_ptr(extract->tape);
    if (!tape) return;
    if (!energy_consume(chunk_energy(chunk), tape_energy(tape))) return;

    struct tape_ret ret = tape_at(tape, tape_packed_it(extract->tape));
    switch (ret.state) {
    case tape_eof: { im_extract_step_eof(extract, chunk); return; }
    case tape_input: { im_extract_step_input(extract, chunk, ret.item); return; }
    case tape_work: { im_extract_step_work(extract, chunk, tape); return; }
    case tape_output: { im_extract_step_output(extract, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_extract_io_state(
        struct im_extract *extract, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, extract->id, IO_STATE, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case IO_TAPE: { value = tape_packed_id(extract->tape); break; }
    case IO_LOOP: { value = extract->loops; break; }
    default: { chunk_log(chunk, extract->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, extract->id, src, &value, 1);
}

static void im_extract_io_tape(
        struct im_extract *extract, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, extract->id, IO_TAPE, len, 1)) return;

    enum item item = args[0];
    if (!item_validate(args[0]))
        return chunk_log(chunk, extract->id, IO_TAPE, IOE_A0_INVALID);

    if (!im_check_known(chunk, extract->id, IO_TAPE, item)) return;

    const struct tape *tape = tapes_get(item);
    if (!tape || tape_host(tape) != im_id_item(extract->id))
        return chunk_log(chunk, extract->id, IO_TAPE, IOE_A0_INVALID);

    im_extract_reset(extract, chunk);
    extract->tape = tape_pack(item, 0, tape);
    extract->loops = im_loops_io(len > 1 ? args[1] : im_loops_inf);
}

static void im_extract_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_extract *extract = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, extract->id, src, NULL, 0); return; }
    case IO_STATE: { im_extract_io_state(extract, chunk, src, args, len); return; }

    case IO_TAPE: { im_extract_io_tape(extract, chunk, args, len); return; }
    case IO_RESET: { im_extract_reset(extract, chunk); return; }

    default: { return; }
    }
}

static const struct io_cmd im_extract_io_list[] =
{
    { IO_PING,  0, {} },
    { IO_STATE, 1, { { "state", true } }},
    { IO_RESET, 0, {} },
    { IO_TAPE,  1, { { "tape", true } }},
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_extract_flow(const void *state, struct flow *flow)
{
    const struct im_extract *extract = state;
    if (!extract->tape) return false;

    enum item target = tape_packed_id(extract->tape);
    const struct tape *tape = tapes_get(target);
    struct tape_ret ret = tape_at(tape, tape_packed_it(extract->tape));

    *flow = (struct flow) {
        .id = extract->id,
        .loops = extract->loops,
        .target = target,
        .state = ret.state,
        .item = ret.state ? ret.item : ITEM_NIL,
        .tape_it = tape_packed_it(extract->tape),
        .tape_len = tape_len(tape),
        .rank = tapes_info(target)->rank,
    };

    return true;
}
