/* legion_im.c
   Rémi Attab (remi.attab@gmail.com), 05 Jul 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/chunk.h"
#include "items/io.h"


// -----------------------------------------------------------------------------
// legion
// -----------------------------------------------------------------------------

static void im_legion_init(void *state, struct chunk *chunk, im_id id)
{
    struct im_legion *legion = state;
    (void) chunk;

    legion->id = id;
}

const enum item *im_legion_cargo(enum item type)
{
    switch (type)
    {
    case item_legion: {
        static const enum item cargo[] = {
            item_worker,
            item_worker,
            item_fusion,
            item_deploy,
            item_extract,
            item_extract,
            item_printer,
            item_printer,
            item_assembly,
            item_assembly,
            item_prober,
            item_memory,
            item_brain,
            item_nil,
        };
        return cargo;
    }

    default: { assert(false); }
    };
}

static void im_legion_make(
        void *state, struct chunk *chunk,
        im_id id,
        const vm_word *data, size_t len)
{
    (void) state;

    vm_word src = len >= 1 ? data[0] : 0;
    vm_word mod = len >= 2 ? data[1] : 0;

    for (const enum item *it = im_legion_cargo(im_id_item(id)); *it; ++it) {

        switch (*it)
        {

        case item_brain:  {
            if (!chunk_create_from(chunk, *it, &mod, src ? 1 : 0))
                chunk_log(chunk, id, IO_ARRIVE, IOE_OUT_OF_SPACE);
            break;
        }

        case item_memory: {
            if (!chunk_create_from(chunk, *it, &src, mod ? 1 : 0))
                chunk_log(chunk, id, IO_ARRIVE, IOE_OUT_OF_SPACE);
            break;
        }

        default: {
            if (!chunk_create(chunk, *it))
                chunk_log(chunk, id, IO_ARRIVE, IOE_OUT_OF_SPACE);
            break;
        }

        }
    }

    chunk_delete(chunk, id);
}

// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_legion_io_state(
        struct im_legion *legion, struct chunk *chunk,
        im_id src,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, legion->id, IO_STATE, len, 1)) return;
    vm_word value = 0;

    switch (args[0]) {
    case IO_MOD: { value = legion->mod; break; }
    default: { chunk_log(chunk, legion->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, legion->id, src, &value, 1);
}

static void im_legion_io_reset(struct im_legion *legion, struct chunk *chunk)
{
    chunk_ports_reset(chunk, legion->id);
    legion->mod = 0;
}

static void im_legion_io_mod(
        struct im_legion *legion, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, legion->id, IO_MOD, len, 1)) return;

    mod_id mod = args[0];
    if (!mod_validate(args[0]))
        return chunk_log(chunk, legion->id, IO_MOD, IOE_A0_INVALID);

    legion->mod = mod;
}

static void im_legion_io_launch(
        struct im_legion *legion, struct chunk *chunk,
        const vm_word *args, size_t len)
{
    if (!im_check_args(chunk, legion->id, IO_LAUNCH, len, 1)) return;

    struct coord dst = coord_from_u64(args[0]);
    if (!coord_validate(args[0]))
        return chunk_log(chunk, legion->id, IO_MOD, IOE_A0_INVALID);

    const vm_word data[] = {
        coord_to_u64(chunk_star(chunk)->coord),
        legion->mod,
    };

    chunk_lanes_launch(
            chunk,
            im_id_item(legion->id),
            im_legion_speed,
            dst,
            data, array_len(data));
    chunk_delete(chunk, legion->id);
}

static void im_legion_io(
        void *state, struct chunk *chunk,
        enum io io, im_id src,
        const vm_word *args, size_t len)
{
    struct im_legion *legion = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, legion->id, src, NULL, 0); return; }
    case IO_STATE: { im_legion_io_state(legion, chunk, src, args, len); return; }

    case IO_MOD: { im_legion_io_mod(legion, chunk, args, len); return; }
    case IO_RESET: { im_legion_io_reset(legion, chunk); return; }

    case IO_LAUNCH: { im_legion_io_launch(legion, chunk, args, len); return; }

    default: { return; }
    }
}

static const struct io_cmd im_legion_io_list[] =
{
    { IO_PING,   0, {} },
    { IO_STATE,  1, { { "state", true } }},
    { IO_RESET,  0, {} },
    { IO_MOD,    1, { { "mod", true } }},
    { IO_LAUNCH, 0, {} },
};
