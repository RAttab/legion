/* lab_im.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "items/io.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// lab
// -----------------------------------------------------------------------------

static void im_lab_init(void *state, struct chunk *chunk, id_t id)
{
    struct im_lab *lab = state;
    (void) chunk;

    lab->id = id;
    lab->state = im_lab_idle;
    lab->rng = rng_make(id);
}

static void im_lab_reset(struct im_lab *lab, struct chunk *chunk)
{
    chunk_ports_reset(chunk, lab->id);
    lab->item = ITEM_NIL;
    lab->state = im_lab_idle;
    lab->work.left = 0;
    lab->work.cap = 0;
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_lab_step(void *state, struct chunk *chunk)
{
    struct im_lab *lab = state;
    if (lab->item == ITEM_NIL) return;

    struct tech *tech = chunk_tech(chunk);

    if (tech_learned(tech, lab->item)) {
        im_lab_reset(lab, chunk);
        return;
    }

    switch (lab->state)
    {

    case im_lab_idle: {
        chunk_ports_request(chunk, lab->id, lab->item);
        lab->state = im_lab_waiting;
        return;
    }

    case im_lab_waiting: {
        enum item consumed = chunk_ports_consume(chunk, lab->id);
        if (!consumed) return;
        assert(consumed == lab->item);

        lab->work.left = lab->work.cap;
        lab->state = im_lab_working;
        return;
    }

    case im_lab_working: {
        lab->work.left--;
        if (lab->work.left) return;

        const uint8_t bits = im_config_assert(lab->item)->lab_bits;
        uint8_t bit = rng_uni(&lab->rng, 0, bits);
        tech_learn_bit(tech, lab->item, bit);

        lab->state = im_lab_idle;
        return;
    }

    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_lab_io_status(
        struct im_lab *lab, struct chunk *chunk, id_t src)
{
    word_t item = lab->item;
    chunk_io(chunk, IO_STATE, lab->id, src, &item, 1);
}

static void im_lab_io_item(
        struct im_lab *lab, struct chunk *chunk,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, lab->id, IO_ITEM, len, 1)) return;

    enum item item = args[0];
    if (!item_validate(args[0]))
        return chunk_log(chunk, lab->id, IO_ITEM, IOE_A0_INVALID);

    if (!im_check_known(chunk, lab->id, IO_ITEM, item)) return;

    const struct im_config *config = im_config(item);
    if (!config)
        return chunk_log(chunk, lab->id, IO_ITEM, IOE_A0_UNKNOWN);

    if (tech_learned(chunk_tech(chunk), item)) return;

    im_lab_reset(lab, chunk);
    lab->item = item;
    lab->work.cap = config->lab_work;
}

static void im_lab_io_tape_at(
        struct im_lab *lab, struct chunk *chunk,
        id_t src,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, lab->id, IO_TAPE_AT, len, 2)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, lab->id, IO_TAPE_AT, IOE_A0_INVALID);
        goto fail;
    }

    if (!im_check_known(chunk, lab->id, IO_TAPE_AT, item)) goto fail;

    const struct tape *tape = tapes_get(item);
    if (!tape) {
        chunk_log(chunk, lab->id, IO_TAPE_AT, IOE_A0_UNKNOWN);
        goto fail;
    }

    tape_it_t index = args[1];
    if (!tape_it_validate(args[1])) {
        chunk_log(chunk, lab->id, IO_TAPE_AT, IOE_A1_INVALID);
        goto fail;
    }

    word_t result = 0;
    struct tape_ret ret = tape_at(tape, index);
    if (ret.state == tape_input || ret.state == tape_output)
        result = vm_pack(ret.item, ret.state);

    chunk_io(chunk, IO_RETURN, lab->id, src, &result, 1);
    return;

  fail:
    {
        word_t fail = 0;
        chunk_io(chunk, IO_RETURN, lab->id, src, &fail, 1);
    }
    return;
}

static void im_lab_io_tape_known(
        struct im_lab *lab, struct chunk *chunk,
        id_t src,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, lab->id, IO_TAPE_KNOWN, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, lab->id, IO_TAPE_KNOWN, IOE_A0_INVALID);
        goto fail;
    }

    struct tech *tech = chunk_tech(chunk);
    word_t known = tech_known(tech, item) ? 1 : 0;
    chunk_io(chunk, IO_RETURN, lab->id, src, &known, 1);
    return;

  fail:
    {
        word_t fail = 0;
        chunk_io(chunk, IO_RETURN, lab->id, src, &fail, 1);
    }
    return;
}

static void im_lab_io_item_bits(
        struct im_lab *lab, struct chunk *chunk,
        id_t src,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, lab->id, IO_ITEM_BITS, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, lab->id, IO_ITEM_BITS, IOE_A0_INVALID);
        goto fail;
    }

    struct tech *tech = chunk_tech(chunk);
    word_t bits = tech_learned_bits(tech, item);
    chunk_io(chunk, IO_RETURN, lab->id, src, &bits, 1);
    return;

  fail:
    {
        word_t fail = 0;
        chunk_io(chunk, IO_RETURN, lab->id, src, &fail, 1);
    }
    return;
}

static void im_lab_io_item_known(
        struct im_lab *lab, struct chunk *chunk,
        id_t src,
        const word_t *args, size_t len)
{
    if (!im_check_args(chunk, lab->id, IO_ITEM_KNOWN, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, lab->id, IO_ITEM_KNOWN, IOE_A0_INVALID);
        goto fail;
    }

    struct tech *tech = chunk_tech(chunk);
    word_t known = tech_learned(tech, item) ? 1 : 0;
    chunk_io(chunk, IO_RETURN, lab->id, src, &known, 1);
    return;

  fail:
    {
        word_t fail = 0;
        chunk_io(chunk, IO_RETURN, lab->id, src, &fail, 1);
    }
    return;
}

static void im_lab_io(
        void *state, struct chunk *chunk,
        enum io io, id_t src,
        const word_t *args, size_t len)
{
    struct im_lab *lab = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, lab->id, src, NULL, 0); return; }
    case IO_STATUS: { im_lab_io_status(lab, chunk, src); return; }

    case IO_ITEM: { im_lab_io_item(lab, chunk, args, len); return; }
    case IO_RESET: { im_lab_reset(lab, chunk); return; }

    case IO_TAPE_AT: { im_lab_io_tape_at(lab, chunk, src, args, len); return; }
    case IO_TAPE_KNOWN: { im_lab_io_tape_known(lab, chunk, src, args, len); return; }
    case IO_ITEM_BITS: { im_lab_io_item_bits(lab, chunk, src, args, len); return; }
    case IO_ITEM_KNOWN: { im_lab_io_item_known(lab, chunk, src, args, len); return; }

    default: { return; }
    }
}

static const word_t im_lab_io_list[] =
{
    IO_PING,
    IO_STATUS,

    IO_ITEM,
    IO_RESET,

    IO_TAPE_AT,
    IO_TAPE_KNOWN,
    IO_ITEM_BITS,
    IO_ITEM_KNOWN,
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_lab_flow(const void *state, struct flow *flow)
{
    const struct im_lab *lab = state;
    if (!lab->item) return false;
    *flow = (struct flow) {
        .id = lab->id,
        .loops = loops_inf,
        .target = lab->item,
        .in = lab->state == im_lab_waiting ? lab->item : 0,
        .rank = tapes_info(lab->item)->rank + 1,
    };

    return true;
}
