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

static void im_extract_init(void *state, struct chunk *chunk, id_t id)
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
    if (extract->loops != loops_inf) extract->loops--;

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

static void im_extract_step_output(
        struct im_extract *extract, struct chunk *chunk, enum item item)
{
    if (!extract->waiting) {
        if (!chunk_harvest(chunk, item)) {
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

    struct tape_ret ret = tape_at(tape, tape_packed_it(extract->tape));
    switch (ret.state) {
    case tape_eof: { im_extract_step_eof(extract, chunk); return; }
    case tape_input: { im_extract_step_input(extract, chunk, ret.item); return; }
    case tape_output: { im_extract_step_output(extract, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_extract_io_status(
        struct im_extract *extract, struct chunk *chunk, id_t src)
{
    word_t value = vm_pack(extract->loops, tape_packed_id(extract->tape));
    chunk_io(chunk, IO_STATE, extract->id, src, &value, 1);
}

static void im_extract_io_tape(
        struct im_extract *extract, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (len < 1) return;

    word_t tape_id = args[0];
    if (tape_id != (enum item) tape_id) return;

    const struct tape *tape = tapes_get(tape_id);
    if (!tape || tape_host(tape) != id_item(extract->id)) return;

    im_extract_reset(extract, chunk);
    extract->tape = tape_pack(tape_id, 0, tape);
    extract->loops = loops_io(len > 1 ? args[1] : loops_inf);
}

static void im_extract_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_extract *extract = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, extract->id, src, NULL, 0); return; }
    case IO_STATUS: { im_extract_io_status(extract, chunk, src); return; }

    case IO_TAPE: { im_extract_io_tape(extract, chunk, args, len); return; }
    case IO_RESET: { im_extract_reset(extract, chunk); return; }

    default: { return; }
    }
}

static const word_t im_extract_io_list[] =
{
    IO_PING,
    IO_STATUS,

    IO_TAPE,
    IO_RESET,
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_extract_flow(const void *state, struct flow *flow)
{
    const struct im_extract *extract = state;
    if (!extract->tape) return false;

    *flow = (struct flow) {
        .id = extract->id,
        .loops = extract->loops,
        .target = tape_packed_id(extract->tape),
    };

    const struct tape *tape = tape_packed_ptr(extract->tape);
    struct tape_ret ret = tape_at(tape, tape_packed_it(extract->tape));

    switch (ret.state) {
    case tape_input: { flow->in = ret.item; break; }
    case tape_output: { flow->out = ret.item; break; }
    default: { break; }
    }

    flow->tape_it = tape_packed_it(extract->tape);
    flow->tape_len = tape_len(tape);
    return true;
}
