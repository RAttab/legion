/* lab_im.c
   Rémi Attab (remi.attab@gmail.com), 14 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "db/io.h"
#include "items/types.h"
#include "game/chunk.h"


// -----------------------------------------------------------------------------
// lab
// -----------------------------------------------------------------------------

static void im_lab_init(void *state, struct chunk *chunk, im_id id)
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
    lab->item = item_nil;
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
    if (lab->item == item_nil) return;

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
        im_energy energy = specs_var_assert(make_spec(lab->item, spec_lab_energy));
        if (!energy_consume(chunk_energy(chunk), energy))
            return;

        lab->work.left--;
        if (lab->work.left) return;

        const uint8_t bits = specs_var_assert(make_spec(lab->item, spec_lab_bits));
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

static void im_lab_io_state(
        struct im_lab *lab, struct chunk *chunk, im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, lab->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case io_item: { value = lab->item; break; }
    default: { chunk_log(chunk, lab->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, lab->id, src, &value, 1);
}

static void im_lab_io_item(
        struct im_lab *lab, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, lab->id, io_item, len, 1)) return;

    enum item item = args[0];
    if (!item_validate(args[0]))
        return chunk_log(chunk, lab->id, io_item, ioe_a0_invalid);

    if (!im_check_known(chunk, lab->id, io_item, item)) return;
    if (tech_learned(chunk_tech(chunk), item)) return;

    im_lab_reset(lab, chunk);
    lab->item = item;
    lab->work.cap = specs_var_assert(make_spec(item, spec_lab_work));
}

static void im_lab_io_tape_at(
        struct im_lab *lab, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, lab->id, io_tape_at, len, 2)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, lab->id, io_tape_at, ioe_a0_invalid);
        goto fail;
    }

    if (!im_check_known(chunk, lab->id, io_tape_at, item)) goto fail;

    const struct tape *tape = tapes_get(item);
    if (!tape) {
        chunk_log(chunk, lab->id, io_tape_at, ioe_a0_unknown);
        goto fail;
    }

    tape_it index = args[1];
    if (!tape_it_validate(args[1])) {
        chunk_log(chunk, lab->id, io_tape_at, ioe_a1_invalid);
        goto fail;
    }

    vm_word result = 0;
    struct tape_ret ret = tape_at(tape, index);
    if (ret.state == tape_input || ret.state == tape_output)
        result = vm_pack(ret.item, ret.state);

    chunk_io(chunk, io_return, lab->id, src, &result, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, lab->id, src, &fail, 1);
    }
    return;
}

static void im_lab_io_tape_known(
        struct im_lab *lab, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, lab->id, io_tape_known, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, lab->id, io_tape_known, ioe_a0_invalid);
        goto fail;
    }

    struct tech *tech = chunk_tech(chunk);
    vm_word known = tech_known(tech, item) ? 1 : 0;
    chunk_io(chunk, io_return, lab->id, src, &known, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, lab->id, src, &fail, 1);
    }
    return;
}

static void im_lab_io_item_bits(
        struct im_lab *lab, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, lab->id, io_item_bits, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, lab->id, io_item_bits, ioe_a0_invalid);
        goto fail;
    }

    struct tech *tech = chunk_tech(chunk);
    vm_word bits = tech_learned_bits(tech, item);
    chunk_io(chunk, io_return, lab->id, src, &bits, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, lab->id, src, &fail, 1);
    }
    return;
}

static void im_lab_io_item_known(
        struct im_lab *lab, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, lab->id, io_item_known, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, lab->id, io_item_known, ioe_a0_invalid);
        goto fail;
    }

    struct tech *tech = chunk_tech(chunk);
    vm_word known = tech_learned(tech, item) ? 1 : 0;
    chunk_io(chunk, io_return, lab->id, src, &known, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, lab->id, src, &fail, 1);
    }
    return;
}

static void im_lab_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_lab *lab = state;

    switch(io)
    {
    case io_ping: { chunk_io(chunk, io_pong, lab->id, src, NULL, 0); return; }
    case io_state: { im_lab_io_state(lab, chunk, src, args, len); return; }

    case io_item: { im_lab_io_item(lab, chunk, args, len); return; }
    case io_reset: { im_lab_reset(lab, chunk); return; }

    case io_tape_at: { im_lab_io_tape_at(lab, chunk, src, args, len); return; }
    case io_tape_known: { im_lab_io_tape_known(lab, chunk, src, args, len); return; }
    case io_item_bits: { im_lab_io_item_bits(lab, chunk, src, args, len); return; }
    case io_item_known: { im_lab_io_item_known(lab, chunk, src, args, len); return; }

    default: { return; }
    }
}

static const struct io_cmd im_lab_io_list[] =
{
    { io_ping,       0, {} },
    { io_state,      1, { { "state", true } }},
    { io_reset,      0, {} },

    { io_item,       1, { { "item", true } }},

    { io_tape_at,    2, { { "item", true },
                          { "index", true } }},
    { io_tape_known, 1, { { "item", true } }},

    { io_item_bits,  1, { { "item", true } }},
    { io_item_known, 1, { { "item", true } }},
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_lab_flow(const void *state, struct flow *flow)
{
    const struct im_lab *lab = state;
    if (!lab->item) return false;

    enum item item = lab->state == im_lab_waiting ? lab->item : item_nil;
    *flow = (struct flow) {
        .id = lab->id,
        .loops = im_loops_inf,
        .target = lab->item,
        .state = item ? tape_input : tape_eof,
        .item = item,
        .rank = tapes_info(lab->item)->rank + 1,
    };

    return true;
}
