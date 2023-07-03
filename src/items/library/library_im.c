/* library_im.c
   Rémi Attab (remi.attab@gmail.com), 28 Jun 2023
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/chunk.h"
#include "db/io.h"


// -----------------------------------------------------------------------------
// library
// -----------------------------------------------------------------------------

static void im_library_init(void *state, struct chunk *chunk, im_id id)
{
    struct im_library *library = state;
    (void) chunk;

    library->id = id;
}


// -----------------------------------------------------------------------------
// IO - Generic
// -----------------------------------------------------------------------------

static void im_library_io_state(
        struct im_library *library, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, library->id, io_state, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case io_tape: { value = library->item; break; }
    case io_item: { value = library->value; break; }
    default: { chunk_log(chunk, library->id, io_state, ioe_a0_invalid); break; }
    }

    chunk_io(chunk, io_return, library->id, src, &value, 1);
}

static void im_library_io_reset(struct im_library *library)
{
    legion_zero_after(library, id);
}


// -----------------------------------------------------------------------------
// IO - Tape
// -----------------------------------------------------------------------------

static void im_library_io_tape_in(
        struct im_library *library, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, library->id, io_tape_in, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, library->id, io_tape_in, ioe_a0_invalid);
        goto fail;
    }

    const struct tape *tape = tapes_get(item);
    if (!tape) {
        chunk_log(chunk, library->id, io_tape_in, ioe_a0_invalid);
        goto fail;
    }

    if (library->op != im_library_in || library->item != item) {
        library->op = im_library_in;
        library->item = item;
        library->index = -1;
        library->len = tape->inputs;
        library->value = item_nil;
    }

    library->index++;
    library->value = tape_input_at(tape, library->index);
    if (!library->value) library->index--;

    vm_word ret = library->value;
    chunk_io(chunk, io_return, library->id, src, &ret, 1);
    return;

  fail:
    {
        vm_word fail = item_nil;
        chunk_io(chunk, io_return, library->id, src, &fail, 1);
    }
    return;
}

static void im_library_io_tape_out(
        struct im_library *library, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, library->id, io_tape_out, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, library->id, io_tape_out, ioe_a0_invalid);
        goto fail;
    }

    const struct tape *tape = tapes_get(item);
    if (!tape) {
        chunk_log(chunk, library->id, io_tape_out, ioe_a0_invalid);
        goto fail;
    }

    if (library->op != im_library_out || library->item != item) {
        library->op = im_library_out;
        library->item = item;
        library->index = -1;
        library->len = tape->outputs;
        library->value = item_nil;
    }

    library->index++;
    library->value = tape_output_at(tape, library->index);
    if (!library->value) library->index--;

    vm_word ret = library->value;
    chunk_io(chunk, io_return, library->id, src, &ret, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, library->id, src, &fail, 1);
    }
    return;
}

static void im_library_io_tape_tech(
        struct im_library *library, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, library->id, io_tape_tech, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, library->id, io_tape_tech, ioe_a0_invalid);
        goto fail;
    }

    const struct tape_info *info = tapes_info(item);
    if (!info) {
        chunk_log(chunk, library->id, io_tape_tech, ioe_a0_invalid);
        goto fail;
    }

    if (library->op != im_library_tech || library->item != item) {
        library->op = im_library_tech;
        library->item = item;
        library->index = -1;
        library->len = tape_set_len(&info->tech);
        library->value = item_nil;
    }

    library->index++;
    library->value = tape_set_at(&info->tech, library->index);
    if (!library->value) library->index--;
    
    vm_word ret = library->value;
    chunk_io(chunk, io_return, library->id, src, &ret, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, library->id, src, &fail, 1);
    }
    return;
}

static void im_library_io_tape_host(
        struct im_library *library, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, library->id, io_tape_host, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, library->id, io_tape_host, ioe_a0_invalid);
        goto fail;
    }

    const struct tape *tape = tapes_get(item);
    if (!tape) {
        chunk_log(chunk, library->id, io_tape_host, ioe_a0_invalid);
        goto fail;
    }

    vm_word ret = tape_host(tape);
    chunk_io(chunk, io_return, library->id, src, &ret, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, library->id, src, &fail, 1);
    }
    return;
}

static void im_library_io_tape_work(
        struct im_library *library, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, library->id, io_tape_work, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, library->id, io_tape_work, ioe_a0_invalid);
        goto fail;
    }

    const struct tape *tape = tapes_get(item);
    if (!tape) {
        chunk_log(chunk, library->id, io_tape_work, ioe_a0_invalid);
        goto fail;
    }

    vm_word ret = tape_work_cap(tape);
    chunk_io(chunk, io_return, library->id, src, &ret, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, library->id, src, &fail, 1);
    }
    return;
}

static void im_library_io_tape_energy(
        struct im_library *library, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, library->id, io_tape_energy, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, library->id, io_tape_energy, ioe_a0_invalid);
        goto fail;
    }

    const struct tape *tape = tapes_get(item);
    if (!tape) {
        chunk_log(chunk, library->id, io_tape_energy, ioe_a0_invalid);
        goto fail;
    }

    vm_word ret = tape_energy(tape);
    chunk_io(chunk, io_return, library->id, src, &ret, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, library->id, src, &fail, 1);
    }
    return;
}

static void im_library_io_tape_known(
        struct im_library *library, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, library->id, io_tape_known, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, library->id, io_tape_known, ioe_a0_invalid);
        goto fail;
    }

    struct tech *tech = chunk_tech(chunk);
    vm_word known = tech_known(tech, item) ? 1 : 0;
    chunk_io(chunk, io_return, library->id, src, &known, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, library->id, src, &fail, 1);
    }
    return;
}

static void im_library_io_tape_learned(
        struct im_library *library, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, library->id, io_tape_learned, len, 1)) goto fail;

    enum item item = args[0];
    if (!item_validate(args[0])) {
        chunk_log(chunk, library->id, io_tape_learned, ioe_a0_invalid);
        goto fail;
    }

    struct tech *tech = chunk_tech(chunk);
    vm_word learned = tech_learned(tech, item) ? 1 : 0;
    chunk_io(chunk, io_return, library->id, src, &learned, 1);
    return;

  fail:
    {
        vm_word fail = 0;
        chunk_io(chunk, io_return, library->id, src, &fail, 1);
    }
    return;
}


// -----------------------------------------------------------------------------
// IO - Setup
// -----------------------------------------------------------------------------

static void im_library_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_library *library = state;

    switch (io)
    {
    case io_ping: { chunk_io(chunk, io_pong, library->id, src, NULL, 0); return; }
    case io_state: { im_library_io_state(library, chunk, src, args, len); return; }
    case io_reset: { im_library_io_reset(library); return; }

    case io_tape_in: { im_library_io_tape_in(library, chunk, src, args, len); return; }
    case io_tape_out: { im_library_io_tape_out(library, chunk, src, args, len); return; }
    case io_tape_tech: { im_library_io_tape_tech(library, chunk, src, args, len); return; }
    case io_tape_host: { im_library_io_tape_host(library, chunk, src, args, len); return; }
    case io_tape_work: { im_library_io_tape_work(library, chunk, src, args, len); return; }
    case io_tape_energy: { im_library_io_tape_energy(library, chunk, src, args, len); return; }
    case io_tape_known: { im_library_io_tape_known(library, chunk, src, args, len); return; }
    case io_tape_learned: { im_library_io_tape_learned(library, chunk, src, args, len); return; }

    default: { return; }
    }
}

static const struct io_cmd im_library_io_list[] =
{
    { io_ping,  0, {} },
    { io_state, 1, { { "state", true } }},
    { io_reset, 0, {} },

    { io_tape_in,   1,    { { "tape", true } }},
    { io_tape_out,  1,    { { "tape", true } }},
    { io_tape_tech,  1,   { { "tape", true } }},
    { io_tape_host, 1,    { { "tape", true } }},
    { io_tape_work, 1,    { { "tape", true } }},
    { io_tape_energy, 1,  { { "tape", true } }},
    { io_tape_known, 1,   { { "tape", true } }},
    { io_tape_learned, 1, { { "tape", true } }},
};
