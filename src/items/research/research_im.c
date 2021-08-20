/* research_im.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "items/io.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// research
// -----------------------------------------------------------------------------

static void im_research_init(void *state, struct chunk *chunk, id_t id)
{
    struct im_research *research = state;
    (void) chunk;

    research->id = id;
    research->state = im_research_idle;
    research->rng = rng_make(id);
}

static void im_research_reset(struct im_research *research, struct chunk *chunk)
{
    chunk_ports_reset(chunk, research->id);
    research->item = ITEM_NIL;
    research->state = im_research_idle;
    research->work.left = 0;
    research->work.cap = 0;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_research_step(void *state, struct chunk *chunk)
{
    struct im_research *research = state;
    if (research->item == ITEM_NIL) return;

    switch (research->state)
    {

    case im_research_idle: {
        chunk_ports_request(chunk, research->id, research->item);
        research->state = im_research_waiting;
        return;
    }

    case im_research_waiting: {
        enum item item = chunk_ports_consume(chunk, research->id);
        assert(item == research->item);

        research->work.left = research->work.cap;
        research->state = im_research_working;
        return;
    }

    case im_research_working: {
        research->work.left--;
        if (research->work.left) return;

        chunk_learn_bit(chunk, research->item, rng_step(&research->rng));
        if (chunk_known(chunk, research->item)) research->item = ITEM_NIL;

        research->state = im_research_idle;
        return;
    }

    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_research_io_status(
        struct im_research *research, struct chunk *chunk, id_t src)
{
    word_t item = research->item;
    chunk_io(chunk, IO_STATE, research->id, src, &item, 1);
}

static void im_research_io_item(
        struct im_research *research, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (len < 1) return;
    if (args[0] < 0 || args[0] >= ITEM_MAX) return;

    enum item item = args[0];
    im_research_reset(research, chunk);

    if (!tapes_get(item) || chunk_known(chunk, item)) return;
    research->item = item;
    research->work.cap = 0x80; // \todo should it be configured and/or dynamic?
}

static void im_research_io_learn(
        struct im_research *, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (len < 1) return;
    chunk_learn(chunk, args[0]);
}

static void im_research_io_tape_data(
        struct im_research *research, struct chunk *chunk,
        id_t src,
        const word_t *args, size_t len)
{
    if (len < 1) goto fail;
    if (args[0] < 0 || args[0] >= ITEM_MAX) goto fail;

    word_t result = chunk_known(chunk, args[0]);
    chunk_io(chunk, IO_TAPE_DATA, research->id, src, &result, 1);
    return;

  fail:
    word_t fail = 0;
    chunk_io(chunk, IO_TAPE_DATA, research->id, src, &fail, 1);
}

static void im_research_io_tape_at(
        struct im_research *research, struct chunk *chunk,
        id_t src,
        const word_t *args, size_t len)
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

    chunk_io(chunk, IO_TAPE_AT, research->id, src, &result, 1);
    return;

  fail:
    word_t fail = 0;
    chunk_io(chunk, IO_TAPE_AT, research->id, src, &fail, 1);
}

static void im_research_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_research *research = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, research->id, src, NULL, 0); return; }
    case IO_STATUS: { im_research_io_status(research, chunk, src); return; }

    case IO_ITEM: { im_research_io_item(research, chunk, args, len); return; }
    case IO_RESET: { im_research_reset(research, chunk); return; }

    case IO_LEARN: { im_research_io_learn(research, chunk, args, len); return; }
    case IO_TAPE_DATA: { im_research_io_tape_data(research, chunk, src, args, len); return; }
    case IO_TAPE_AT: { im_research_io_tape_at(research, chunk, src, args, len); return; }

    default: { return; }
    }
}

static const word_t im_research_io_list[] =
{
    IO_PING,
    IO_STATUS,

    IO_ITEM,
    IO_RESET,

    IO_LEARN,
    IO_TAPE_DATA,
    IO_TAPE_AT,
};
