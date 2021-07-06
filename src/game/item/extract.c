/* extract.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/atoms.h"
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

    prog_id_t id = prog_packed_id(extract->prog);
    if (!id) return;

    const struct prog *prog = prog_fetch(id);
    assert(prog);
    deploy->prog = prog_packed_ptr_update(extract->prog, prog);
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void extract_step_eof(struct extract *extract)
{
    if (extract->loops != extract_loops_inf) extract->loops--;
    if (extract->loops) extract->prog = prog_packed_it_zero(extract->prog);
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

    extract->waitng = false;
    extract->prog = prog_packed_it_inc(extract->prog);
}

static void extract_step_output(
        struct extract *extract, struct chunk *chunk, enum item item)
{
    if (!extract->waiting && !chunk_harvest(chunk, item)) return;

    if (chunk_ports_produce(chunk, extract->id, item))
        extract->prog = prog_packed_it_inc(extract->prog);
    else extract->waiting = true;
}

static void extract_step(void *state, struct chunk *chunk)
{
    struct extract *extract = state;

    const struct prog *prog = prog_packed_ptr(extract->prog);
    if (!prog || extract->state == extract_error) return;

    struct prog_ret ret = prog_at(prog, prog_packed_it(extract->prog));
    switch (ret.state) {
    case prog_eof: { extract_step_eof(extract); return; }
    case prog_input: { extract_step_input(extract, chunk, ret.item); return; }
    case prog_output: { extract_step_output(extract, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void extract_io_reset(struct extract *extract, struct chunk *chunk)
{
    chunk_ports_reset(chunk, progable->id);
    extract->waiting = false;
    extract->loops = 0;
    extract->prog = 0;
}

static void extract_io_prog(
        struct extract *extract, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    word_t prog_id = args[0];
    if (prog_id != (prog_id_t) prog_id) return;

    struct prog *prog = prog_fetch(prog_id);
    if (!prog) return;
    
    extract_io_reset(extract, chunk);
    extract->prog = prog_pack(prog_id, 0, prog);
    extract->loops = loops_io(len > 1 ? args[1] : loops_inf);
}

static void extract_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct extract *extract = state;

    switch(io) {
    case IO_PING: { chunk_io(chunk, IO_PONG, extract->id, src, 0, NULL); return; }
    case IO_PROG: { extract_io_prog(extract, chunk, len, args); return; }
    case IO_RESET: { extract_io_reset(extract); return; }
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct item_config *extract_config(enum item item)
{
    (void) item;
    static const word_t io_list[] = { IO_PING, IO_PROG, IO_RESET };

    static const struct item_config config = {
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
