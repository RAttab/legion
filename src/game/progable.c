/* progable.c
   Rémi Attab (remi.attab@gmail.com), 06 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/active.h"
#include "game/atoms.h"
#include "game/item.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// progable
// -----------------------------------------------------------------------------

static void progable_init(void *state, id_t id, struct chunk *chunk)
{
    struct progable *progable = state;
    (void) chunk;

    *progable = (struct progable) { .id = id };
}

// This crap is awful but it's either that or double the size of the struct
// so... yeah... If you're currently trying to port this code, I regret nothing!

const struct prog *progable_prog(struct progable *progable)
{
    return (const struct prog *) (progable->prog & ((1UL << 48) - 1));
}

static prog_id_t progable_prog_id(struct progable *progable)
{
    return progable->prog >> 48;
}

static void progable_set_prog(struct progable *progable, const struct prog *prog)
{
    uint64_t raw = (uintptr_t) prog;
    assert(!(raw >> 48));

    uint64_t id = prog_id(prog);
    progable->prog = raw | (id << 48);
}


static void progable_load(void *state, struct chunk *chunk)
{
    struct progable *progable = state;
    (void) chunk;

    prog_id_t id = progable_prog_id(progable);
    if (!id) return;

    const struct prog *prog = prog_fetch(id);
    assert(prog);

    progable_set_prog(progable, prog);
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void progable_step_eof(struct progable *progable)
{
    if (progable->loops != progable_loops_inf) progable->loops--;
    if (progable->loops) progable->index = 0;
}

static void progable_step_input(
        struct progable *progable, struct chunk *chunk, item_t item)
{
    if (progable->state != progable_blocked) {
        chunk_ports_request(chunk, progable->id, item);
        progable->state = progable_blocked;
        return;
    }

    item_t consumed = chunk_ports_consume(chunk, progable->id);
    if (consumed == ITEM_NIL) return;

    assert(consumed == item);
    progable->state = progable_nil;
    progable->index++;
}

static void progable_step_output(
        struct progable *progable, struct chunk *chunk, item_t item)
{
    if (id_item(progable->id) == ITEM_MINER) {
        if (progable->state != progable_blocked && !chunk_harvest(chunk, item)) {
            progable->state = progable_error;
            return;
        }
    }

    if (id_item(progable->id) == ITEM_DEPLOYER) {
        chunk_create(chunk, item);
        progable->index++;
    }
    else if (chunk_ports_produce(chunk, progable->id, item)) {
        progable->state = progable_nil;
        progable->index++;
    }
    else progable->state = progable_blocked;
}

static void progable_step(void *state, struct chunk *chunk)
{
    struct progable *progable = state;

    const struct prog *prog = progable_prog(progable);
    if (!prog || progable->state == progable_error) return;

    struct prog_ret ret = prog_at(prog, progable->index);
    switch (ret.state) {
    case prog_eof: { progable_step_eof(progable); return; }
    case prog_input: { progable_step_input(progable, chunk, ret.item); return; }
    case prog_output: { progable_step_output(progable, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void progable_io_reset(struct progable *progable, struct chunk *chunk)
{
    chunk_ports_reset(chunk, progable->id);
    *progable = (struct progable) {
        .id = progable->id,
        .state = progable_nil,
        .loops = 0,
        .index = 0,
        .prog = 0,
    };
}

static void progable_io_prog(
        struct progable *progable, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    word_t id = args[0];
    if (id != (prog_id_t) id) return;

    word_t loops = len > 1 ? args[1] : 0;
    if (!loops) loops = progable_loops_inf;

    const struct prog *prog = prog_fetch(id);
    if (!prog || prog_host(prog) != id_item(progable->id)) return;

    progable_io_reset(progable, chunk);
    progable_set_prog(progable, prog);
    progable->loops = loops;
}

static void progable_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct progable *progable = state;
    (void) src;

    if (io == IO_PING) { chunk_io(chunk, IO_PONG, progable->id, src, 0, NULL); return; }
    if (io == IO_PROG) { progable_io_prog(progable, chunk, len, args); return; }
    if (io == IO_RESET) { progable_io_reset(progable, chunk); return; }
    return;
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct item_config *progable_config(item_t item)
{
    (void) item;
    static const word_t io_list[] = { IO_PING, IO_PROG, IO_RESET };

    static const struct item_config config = {
        .size = sizeof(struct progable),
        .init = progable_init,
        .load = progable_load,
        .step = progable_step,
        .io = progable_io,
        .io_list = io_list,
        .io_list_len = array_len(io_list),
    };
    return &config;
}
