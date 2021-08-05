/* research.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/io.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/active.h"

// -----------------------------------------------------------------------------
// research
// -----------------------------------------------------------------------------

static void research_init(void *state, id_t id, struct chunk *chunk)
{
    struct research *research = state;
    (void) chunk;

    research->id = id;
    research->state = research_idle;
    research->rng = rng_make(id);
}


static void research_reset(struct research *research, struct chunk *chunk)
{
    chunk_ports_reset(chunk, research->id);
    research->item = ITEM_NIL;
    research->state = research_idle;
    research->work.left = 0;
    research->work.cap = 0;
}

// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------


static void research_step(void *state, struct chunk *chunk)
{
    struct research *research = state;
    if (research->item == ITEM_NIL) return;

    switch (research->state)
    {

    case research_idle: {
        chunk_ports_request(chunk, research->id, research->item);
        research->state = research_waiting;
        return;
    }

    case research_waiting: {
        enum item item = chunk_ports_consume(chunk, research->id);
        assert(item == research->item);

        research->work.left = research->work.cap;
        research->state = research_working;
        return;
    }

    case research_working: {
        research->work.left--;
        if (research->work.left) return;

        chunk_learn_bit(chunk, research->item, rng_step(&research->rng));
        if (chunk_known(chunk, research->item)) research->item = ITEM_NIL;

        research->state = research_idle;
        return;
    }

    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void research_io_status(struct research *research, struct chunk *chunk, id_t src)
{
    word_t item = research->item;
    chunk_io(chunk, IO_STATE, research->id, src, 1, &item);
}

static void research_io_item(
        struct research *research, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;
    if (args[0] < 0 || args[0] >= ITEM_MAX) return;

    enum item item = args[0];
    research_reset(research, chunk);

    if (!tapes_get(item) || chunk_known(chunk, item)) return;
    research->item = item;
    research->work.cap = 0x80; // \todo should it be configured and/or dynamic?
}

static void research_io_learn(
        struct research *, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;
    chunk_learn(chunk, args[0]);
}

static void research_io_tape_data(
        struct research *research, struct chunk *chunk,
        id_t src, size_t len, const word_t *args)
{
    if (len < 1) goto fail;
    if (args[0] < 0 || args[0] >= ITEM_MAX) goto fail;

    word_t result = chunk_known(chunk, args[0]);
    chunk_io(chunk, IO_TAPE_DATA, research->id, src, 1, &result);
    return;

  fail:
    word_t fail = 0;
    chunk_io(chunk, IO_TAPE_DATA, research->id, src, 1, &fail);
}

static void research_io_tape_at(
        struct research *research, struct chunk *chunk,
        id_t src, size_t len, const word_t *args)
{
    if (len < 2) goto fail;

    if (args[0] < 0 || args[0] >= ITEM_MAX) goto fail;
    enum item item = args[0];


    if (args[1] < 0 || args[1] >= UINT8_MAX) goto fail;
    tape_it_t index = args[1];

    const struct tape *tape = tapes_get(item);
    if (!tape) goto fail;

    struct tape_ret ret = tape_at(tape, index);

    word_t result = 0;
    if (ret.state == tape_input || ret.state == tape_output)
        result = vm_pack(ret.item, ret.state);

    chunk_io(chunk, IO_TAPE_AT, research->id, src, 1, &result);
    return;

  fail:
    word_t fail = 0;
    chunk_io(chunk, IO_TAPE_AT, research->id, src, 1, &fail);
}

static void research_io(
        void *state, struct chunk *chunk,
        enum atom_io io, id_t src, size_t len, const word_t *args)
{
    struct research *research = state;

    switch(io) {
    case IO_PING: { chunk_io(chunk, IO_PONG, research->id, src, 0, NULL); return; }
    case IO_STATUS: { research_io_status(research, chunk, src); return; }
    case IO_ITEM: { research_io_item(research, chunk, len, args); return; }
    case IO_LEARN: { research_io_learn(research, chunk, len, args); return; }
    case IO_TAPE_DATA: { research_io_tape_data(research, chunk, src, len, args); return; }
    case IO_TAPE_AT: { research_io_tape_at(research, chunk, src, len, args); return; }
    case IO_RESET: { research_reset(research, chunk); return; }
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct active_config *research_config(enum item item)
{
    (void) item;
    static const word_t io_list[] = {
        IO_PING, IO_STATUS, IO_ITEM, IO_RESET,
        IO_LEARN, IO_TAPE_DATA, IO_TAPE_AT,
    };

    static const struct active_config config = {
        .size = sizeof(struct research),
        .init = research_init,
        .step = research_step,
        .io = research_io,
        .io_list = io_list,
        .io_list_len = array_len(io_list),
    };
    return &config;
}
