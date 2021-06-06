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
        chunk_io_request(chunk, progable->id, item);
        progable->state = progable_blocked;
        return;
    }

    item_t consumed = chunk_io_consume(chunk, progable->id);
    if (consumed == ITEM_NIL) return;

    assert(consumed == item);
    progable->state = progable_nil;
    progable->index++;
}

static void progable_step_output(
        struct progable *progable, struct chunk *chunk, item_t item)
{
    if (id_item(progable->id) == ITEM_MINER) {
        if (progable->state != progable_blocked
                && !chunk_harvest(chunk, item))
        {
            progable->state = progable_error;
            return;
        }
    }

    if (id_item(progable->id) == ITEM_DEPLOYER) {
        chunk_create(chunk, item);
        progable->index++;
    }
    else if (chunk_io_produce(chunk, progable->id, item)) {
        progable->state = progable_nil;
        progable->index++;
    }
    else progable->state = progable_blocked;
}

static void progable_step(void *state, struct chunk *chunk)
{
    struct progable *progable = state;
    if (!progable->prog || progable->state == progable_error) return;

    struct prog_ret ret = prog_at(progable->prog, progable->index);
    switch (ret.state) {
    case prog_eof: { progable_step_eof(progable); return; }
    case prog_input: { progable_step_input(progable, chunk, ret.item); return; }
    case prog_output: { progable_step_output(progable, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

static void progable_cmd_reset(struct progable *progable, struct chunk *chunk)
{
    chunk_io_reset(chunk, progable->id);
    *progable = (struct progable) {
        .id = progable->id,
        .state = progable_nil,
        .loops = 0,
        .prog = NULL,
        .index = 0,
    };
}

static void progable_cmd_prog(
        struct progable *progable, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    uint32_t id, loops;
    vm_unpack(args[0], &id, &loops);

    if (!loops) loops = progable_loops_inf;
    if (id != (prog_id_t) id) return;

    const struct prog *prog = prog_fetch(id);
    if (!prog || prog_host(prog) != id_item(progable->id)) return;

    progable_cmd_reset(progable, chunk);
    progable->loops = loops;
    progable->prog = prog;
}

static void progable_cmd(
        void *state, struct chunk *chunk,
        enum atom_io cmd, id_t src, size_t len, const word_t *args)
{
    struct progable *progable = state;
    (void) src;

    if (cmd == IO_PING) { chunk_cmd(chunk, IO_PONG, progable->id, src, 0, NULL); return; }
    if (cmd == IO_PROG) { progable_cmd_prog(progable, chunk, len, args); return; }
    if (cmd == IO_RESET) { progable_cmd_reset(progable, chunk); return; }
    return;
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct item_config *progable_config(item_t item)
{
    (void) item;

    static const struct item_config config = {
        .size = sizeof(struct progable),
        .init = progable_init,
        .step = progable_step,
        .cmd = progable_cmd,
    };
    return &config;
}
