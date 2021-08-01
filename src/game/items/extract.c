/* extract.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/io.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/active.h"


// -----------------------------------------------------------------------------
// extract
// -----------------------------------------------------------------------------

static void extract_init(void *state, id_t id, struct chunk *chunk)
{
    struct extract *extract = state;
    (void) chunk;

    extract->id = id;
}

static void extract_load(void *state, struct chunk *chunk)
{
    struct extract *extract = state;
    (void) chunk;

    enum item id = tape_packed_id(extract->tape);
    if (!id) return;

    const struct tape *tape = tape_fetch(id);
    assert(tape);
    extract->tape = tape_packed_ptr_update(extract->tape, tape);
}

static void extract_reset(struct extract *extract, struct chunk *chunk)
{
    chunk_ports_reset(chunk, extract->id);
    extract->waiting = false;
    extract->loops = 0;
    extract->tape = 0;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void extract_step_eof(struct extract *extract, struct chunk *chunk)
{
    if (extract->loops != loops_inf) extract->loops--;

    if (!extract->loops) extract_reset(extract, chunk);
    else extract->tape = tape_packed_it_zero(extract->tape);
}

static void extract_step_input(
        struct extract *extract, struct chunk *chunk, enum item item)
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

static void extract_step_output(
        struct extract *extract, struct chunk *chunk, enum item item)
{
    if (!extract->waiting) {
        if (!chunk_harvest(chunk, item)) {
            extract_reset(extract, chunk);
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

static void extract_step(void *state, struct chunk *chunk)
{
    struct extract *extract = state;

    const struct tape *tape = tape_packed_ptr(extract->tape);
    if (!tape) return;

    struct tape_ret ret = tape_at(tape, tape_packed_it(extract->tape));
    switch (ret.state) {
    case tape_eof: { extract_step_eof(extract, chunk); return; }
    case tape_input: { extract_step_input(extract, chunk, ret.item); return; }
    case tape_output: { extract_step_output(extract, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void extract_io_status(struct extract *extract, struct chunk *chunk, id_t src)
{
    word_t value = vm_pack(extract->loops, tape_packed_id(extract->tape));
    chunk_io(chunk, IO_STATE, extract->id, src, 1, &value);
}

static void extract_io_tape(
        struct extract *extract, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    word_t tape_id = args[0];
    if (tape_id != (enum item) tape_id) return;

    const struct tape *tape = tape_fetch(tape_id);
    if (!tape || tape->host != id_item(extract->id)) return;

    extract_reset(extract, chunk);
    extract->tape = tape_pack(tape_id, 0, tape);
    extract->loops = loops_io(len > 1 ? args[1] : loops_inf);
}

static void extract_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct extract *extract = state;

    switch(io) {
    case IO_PING: { chunk_io(chunk, IO_PONG, extract->id, src, 0, NULL); return; }
    case IO_STATUS: { extract_io_status(extract, chunk, src); return; }
    case IO_TAPE: { extract_io_tape(extract, chunk, len, args); return; }
    case IO_RESET: { extract_reset(extract, chunk); return; }
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct active_config *extract_config(enum item item)
{
    (void) item;
    static const word_t io_list[] = { IO_PING, IO_STATUS, IO_TAPE, IO_RESET };

    static const struct active_config config = {
        .size = sizeof(struct extract),
        .init = extract_init,
        .load = extract_load,
        .step = extract_step,
        .io = extract_io,
        .io_list = io_list,
        .io_list_len = array_len(io_list),
    };
    return &config;
}
