/* nomad_im.c
   Rémi Attab (remi.attab@gmail.com), 17 Jul 2022
   FreeBSD-style copyright and disclaimer apply

   Nomad has to be a packer as well because otherwise the packer can't pack
   itself and packing the workers would be problematic. Also the brain needs to
   be packed on launch so launch also needs packing capabilities... Lots and
   lots of fun edge cases sadly...

*/

#include "items/io.h"
#include "items/item.h"
#include "items/types.h"
#include "game/chunk.h"
#include "utils/bits.h"


// -----------------------------------------------------------------------------
// cargo
// -----------------------------------------------------------------------------

static size_t im_nomad_cargo_count(struct im_nomad *nomad)
{
    size_t n = 0;
    for (size_t i = 0; i < array_len(nomad->cargo); ++i) {
        struct im_nomad_cargo *cargo = nomad->cargo + i;
        if (cargo->item) n++;
    }
    return n;
}

static struct im_nomad_cargo *im_nomad_cargo_load(
        struct im_nomad *nomad, enum item item)
{
    for (size_t i = 0; i < array_len(nomad->cargo); ++i) {
        struct im_nomad_cargo *cargo = nomad->cargo + i;
        if (!cargo->item || cargo->item == item) return cargo;
    }
    return NULL;
}

static struct im_nomad_cargo *im_nomad_cargo_unload(
        struct im_nomad *nomad, enum item item)
{
    for (size_t i = 0; i < array_len(nomad->cargo); ++i) {
        struct im_nomad_cargo *cargo = nomad->cargo + i;
        if (cargo->item == item) return cargo;
    }
    return NULL;
}

static void im_nomad_cargo_inc(struct im_nomad_cargo *cargo, enum item item)
{
    assert(!cargo->item || cargo->item == item);
    assert(cargo->count < im_nomad_cargo_max);

    cargo->item = item;
    cargo->count++;
}

static void im_nomad_cargo_dec(struct im_nomad_cargo *cargo, enum item item)
{
    assert(cargo->item == item);
    assert(cargo->count);

    cargo->count--;
    if (!cargo->count) cargo->item = ITEM_NIL;
}

// -----------------------------------------------------------------------------
// nomad
// -----------------------------------------------------------------------------

static void im_nomad_port_reset(struct im_nomad *nomad, struct chunk *chunk)
{
    chunk_ports_reset(chunk, nomad->id);
    nomad->op = im_nomad_nil;
    nomad->item = ITEM_NIL;
    nomad->loops = 0;
    nomad->waiting = false;
}

static void im_nomad_port_setup(
        struct im_nomad *nomad,
        struct chunk *chunk,
        enum im_nomad_op op,
        enum item item,
        loops_t loops)
{
    chunk_ports_reset(chunk, nomad->id);
    nomad->op = op;
    nomad->item = item;
    nomad->loops = loops;
    nomad->waiting = false;
}

static void im_nomad_init(void *state, struct chunk *chunk, id id)
{
    (void) chunk;

    struct im_nomad *nomad = state;
    nomad->id = id;
}

static void im_nomad_reset(struct im_nomad *nomad, struct chunk *chunk)
{
    im_nomad_port_reset(nomad, chunk);
    legion_zero_from(nomad, mod);
}


enum
{
    im_nomad_data_cargo = 4,
    im_nomad_data_len =
      1 + im_nomad_memory_len + (im_nomad_cargo_len / im_nomad_data_cargo),
};

static_assert(
        sizeof(struct im_nomad_cargo) * im_nomad_data_cargo ==
        sizeof(word));

static word im_nomad_encode_cargo(struct im_nomad *nomad, size_t ix)
{
    word word = 0;
    assert(ix * im_nomad_data_cargo < array_len(nomad->cargo));
    memcpy(&word, nomad->cargo + (ix * im_nomad_data_cargo), sizeof(word));
    return word;
}

static void im_nomad_decode_cargo(struct im_nomad *nomad, size_t ix, word word)
{
    assert(ix * im_nomad_data_cargo < array_len(nomad->cargo));
    memcpy(nomad->cargo + (ix * im_nomad_data_cargo), &word, sizeof(word));
}

static void im_nomad_make(
        void *state, struct chunk *chunk, id id, const word *data, size_t len)
{
    assert(len == im_nomad_data_len);

    struct im_nomad *nomad = state;
    nomad->id = id;
    {
        nomad->mod = data[0];
        nomad->memory[0] = data[1];
        nomad->memory[1] = data[2];
        nomad->memory[2] = data[3];
        im_nomad_decode_cargo(nomad, 0, data[4]);
        im_nomad_decode_cargo(nomad, 1, data[5]);
        im_nomad_decode_cargo(nomad, 2, data[6]);
    }

    word mod = nomad->mod;

    for (size_t i = 0; i < im_nomad_cargo_len; ++i) {
        struct im_nomad_cargo *cargo = nomad->cargo + i;

        while (cargo->item) {
            switch (cargo->item)
            {

            case ITEM_BRAIN: {
                if (!chunk_create_from(chunk, cargo->item, &mod, mod ? 1 : 0))
                    chunk_log(chunk, nomad->id, IO_ARRIVE, IOE_OUT_OF_SPACE);
                else mod = 0;
                break;
            }

            default: {
                if (!chunk_create(chunk, cargo->item))
                    chunk_log(chunk, nomad->id, IO_ARRIVE, IOE_OUT_OF_SPACE);
                break;
            }

            }

            im_nomad_cargo_dec(cargo, cargo->item);
        }
    }
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void im_nomad_step_pack(struct im_nomad *nomad, struct chunk *chunk)
{
    id id = chunk_last(chunk, nomad->item);
    if (!id) { im_nomad_port_reset(nomad, chunk); return; }

    bool ok = chunk_delete(chunk, id);
    assert(ok);

    struct im_nomad_cargo *cargo = im_nomad_cargo_load(nomad, nomad->item);
    im_nomad_cargo_inc(cargo, nomad->item);

    nomad->loops--;
    if (!nomad->loops) im_nomad_port_reset(nomad, chunk);
}

static void im_nomad_step_load(struct im_nomad *nomad, struct chunk *chunk)
{
    if (!nomad->waiting) {
        chunk_ports_request(chunk, nomad->id, nomad->item);
        nomad->waiting = true;
        return;
    }

    if (!chunk_ports_consume(chunk, nomad->id)) return;

    struct im_nomad_cargo *cargo = im_nomad_cargo_load(nomad, nomad->item);
    im_nomad_cargo_inc(cargo, nomad->item);

    nomad->waiting = false;
    nomad->loops--;
    if (!nomad->loops) im_nomad_port_reset(nomad, chunk);
}

static void im_nomad_step_unload(struct im_nomad *nomad, struct chunk *chunk)
{
    if (!nomad->waiting) {
        chunk_ports_produce(chunk, nomad->id, nomad->item);
        nomad->waiting = true;
        return;
    }

    if (!chunk_ports_consumed(chunk, nomad->id)) return;

    struct im_nomad_cargo *cargo = im_nomad_cargo_load(nomad, nomad->item);
    im_nomad_cargo_dec(cargo, nomad->item);

    nomad->waiting = false;
    nomad->loops--;
    if (!nomad->loops) im_nomad_port_reset(nomad, chunk);
}

static void im_nomad_step(void *state, struct chunk *chunk)
{
    struct im_nomad *nomad = state;
    switch (nomad->op)
    {
    case im_nomad_pack: { return im_nomad_step_pack(nomad, chunk); }
    case im_nomad_load: { return im_nomad_step_load(nomad, chunk); }
    case im_nomad_unload: { return im_nomad_step_unload(nomad, chunk); }
    case im_nomad_nil:
    default: { return; }
    }
}


// -----------------------------------------------------------------------------
// io
// -----------------------------------------------------------------------------

static void im_nomad_io_state(
        struct im_nomad *nomad, struct chunk *chunk, id src,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, nomad->id, IO_STATE, len, 1)) return;
    word value = 0;

    switch (args[0]) {
    case IO_MOD: { value = nomad->mod; break; }
    case IO_ITEM: { value = nomad->item; break; }
    case IO_LOOP: { value = nomad->loops; break; }

    case IO_CARGO: {
        if (len == 1) {
            value = im_nomad_cargo_count(nomad);
            break;
        }

        enum item item = args[1];
        if (!item_validate(args[1])) {
            chunk_log(chunk, nomad->id, IO_STATE, IOE_A1_INVALID);
            break;
        }

        const struct im_nomad_cargo *cargo = im_nomad_cargo_unload(nomad, item);
        if (cargo) value = cargo->count;
        break;
    }

    default: { chunk_log(chunk, nomad->id, IO_STATE, IOE_A0_INVALID); break; }
    }

    chunk_io(chunk, IO_RETURN, nomad->id, src, &value, 1);
}

static void im_nomad_io_id(
        struct im_nomad *nomad, struct chunk *chunk,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, nomad->id, IO_ID, len, 1)) return;

    id id = args[0];
    enum item item = id_item(id);

    if (!id_validate(args[0]))
        return chunk_log(chunk, nomad->id, IO_ID, IOE_A0_INVALID);

    if (!item_is_active(item) && !item_is_logistics(item))
        return chunk_log(chunk, nomad->id, IO_ID, IOE_A0_INVALID);

    if (!im_check_known(chunk, nomad->id, IO_ID, item)) return;

    struct im_nomad_cargo *cargo = im_nomad_cargo_load(nomad, item);
    if (!cargo || cargo->count == im_nomad_cargo_max)
        return chunk_log(chunk, nomad->id, IO_ID, IOE_OUT_OF_SPACE);

    if (!chunk_delete(chunk, id))
        return chunk_log(chunk, nomad->id, IO_ID, IOE_A0_INVALID);

    im_nomad_cargo_inc(cargo, nomad->item);
}

static void im_nomad_io_pack(
        struct im_nomad *nomad, struct chunk *chunk,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, nomad->id, IO_PACK, len, 1)) return;

    enum item item = args[0];

    if (!item_validate(args[0]))
        return chunk_log(chunk, nomad->id, IO_PACK, IOE_A0_INVALID);

    if (!item_is_active(item) && !item_is_logistics(item))
        return chunk_log(chunk, nomad->id, IO_PACK, IOE_A0_INVALID);

    if (!im_check_known(chunk, nomad->id, IO_PACK, item)) return;

    struct im_nomad_cargo *cargo = im_nomad_cargo_load(nomad, item);
    if (!cargo) return chunk_log(chunk, nomad->id, IO_PACK, IOE_OUT_OF_SPACE);

    loops_t loops = loops_io(len > 1 ? args[1] : loops_inf);
    loops = legion_min(loops, im_nomad_cargo_max - cargo->count);
    if (!loops) return chunk_log(chunk, nomad->id, IO_PACK, IOE_OUT_OF_SPACE);

    im_nomad_port_setup(nomad, chunk, im_nomad_pack, item, loops);
}

static void im_nomad_io_load(
        struct im_nomad *nomad, struct chunk *chunk,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, nomad->id, IO_LOAD, len, 1)) return;

    enum item item = args[0];
    if (!item_validate(args[0]))
        return chunk_log(chunk, nomad->id, IO_LOAD, IOE_A0_INVALID);

    if (!item_is_active(item) && !item_is_logistics(item))
        return chunk_log(chunk, nomad->id, IO_LOAD, IOE_A0_INVALID);

    if (!im_check_known(chunk, nomad->id, IO_LOAD, item)) return;

    struct im_nomad_cargo *cargo = im_nomad_cargo_load(nomad, item);
    if (!cargo) return chunk_log(chunk, nomad->id, IO_LOAD, IOE_OUT_OF_SPACE);

    loops_t loops = loops_io(len > 1 ? args[1] : loops_inf);
    loops = legion_min(loops, im_nomad_cargo_max - cargo->count);
    if (!loops) return chunk_log(chunk, nomad->id, IO_LOAD, IOE_OUT_OF_SPACE);

    im_nomad_port_setup(nomad, chunk, im_nomad_load, item, loops);
}

static void im_nomad_io_unload(
        struct im_nomad *nomad, struct chunk *chunk,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, nomad->id, IO_UNLOAD, len, 1)) return;

    enum item item = args[0];

    if (!item_validate(args[0]))
        return chunk_log(chunk, nomad->id, IO_UNLOAD, IOE_A0_INVALID);

    if (!item_is_active(item) && !item_is_logistics(item))
        return chunk_log(chunk, nomad->id, IO_UNLOAD, IOE_A0_INVALID);

    if (!im_check_known(chunk, nomad->id, IO_UNLOAD, item)) return;

    struct im_nomad_cargo *cargo = im_nomad_cargo_unload(nomad, item);
    if (!cargo) return chunk_log(chunk, nomad->id, IO_UNLOAD, IOE_A0_INVALID);

    im_nomad_port_setup(nomad, chunk, im_nomad_unload, item, cargo->count);
}

static void im_nomad_io_mod(
        struct im_nomad *nomad, struct chunk *chunk,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, nomad->id, IO_MOD, len, 1)) return;

    mod_id mod = args[0];
    if (!mod_validate(args[0]))
        return chunk_log(chunk, nomad->id, IO_MOD, IOE_A0_INVALID);

    nomad->mod = mod;
}

static void im_nomad_io_get(
        struct im_nomad *nomad, struct chunk *chunk,
        id src,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, nomad->id, IO_GET, len, 1)) goto fail;

    uint8_t index = args[0];
    if (args[0] < 0 || (size_t) args[0] >= array_len(nomad->memory)) {
        chunk_log(chunk, nomad->id, IO_GET, IOE_A0_INVALID);
        goto fail;
    }

    word value = nomad->memory[index];
    chunk_io(chunk, IO_RETURN, nomad->id, src, &value, 1);
    return;

  fail:
    {
        word fail = 0;
        chunk_io(chunk, IO_RETURN, nomad->id, src, &fail, 1);
    }
    return;
}

static void im_nomad_io_set(
        struct im_nomad *nomad, struct chunk *chunk,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, nomad->id, IO_SET, len, 2)) return;

    uint8_t index = args[0];
    if (args[0] < 0 || (size_t) args[0] >= array_len(nomad->memory))
        return chunk_log(chunk, nomad->id, IO_GET, IOE_A0_INVALID);

    nomad->memory[index] = args[1];
}


static void im_nomad_io_launch(
        struct im_nomad *nomad, struct chunk *chunk,
        const word *args, size_t len)
{
    if (!im_check_args(chunk, nomad->id, IO_LAUNCH, len, 1)) return;

    struct coord dst = coord_from_u64(args[0]);
    if (!coord_validate(args[0]))
        return chunk_log(chunk, nomad->id, IO_MOD, IOE_A0_INVALID);

    // Given that a brain can't both be packed and still be able to issue
    // IO_LAUNCH, IO_LAUNCH has an optional argument that acts like IO_ID prior
    // to launching.
    if (len > 1) {
        id id = args[1];
        enum item item = id_item(id);

        if (!id_validate(args[1]))
            return chunk_log(chunk, nomad->id, IO_LAUNCH, IOE_A1_INVALID);

        if (!item_is_active(item) && !item_is_logistics(item))
            return chunk_log(chunk, nomad->id, IO_LAUNCH, IOE_A1_INVALID);

        if (!im_check_known(chunk, nomad->id, IO_LAUNCH, item)) return;

        struct im_nomad_cargo *cargo = im_nomad_cargo_load(nomad, item);
        if (!cargo || cargo->count == im_nomad_cargo_max)
            return chunk_log(chunk, nomad->id, IO_LAUNCH, IOE_OUT_OF_SPACE);

        if (!chunk_delete(chunk, id))
            return chunk_log(chunk, nomad->id, IO_LAUNCH, IOE_A1_INVALID);

        im_nomad_cargo_inc(cargo, item);
    }

    const word data[im_nomad_data_len] = {
        nomad->mod,
        nomad->memory[0],
        nomad->memory[1],
        nomad->memory[2],
        im_nomad_encode_cargo(nomad, 0),
        im_nomad_encode_cargo(nomad, 1),
        im_nomad_encode_cargo(nomad, 2),
    };

    chunk_lanes_launch(
            chunk, id_item(nomad->id), im_nomad_speed, dst, data, array_len(data));
    chunk_delete(chunk, nomad->id);
}


static void im_nomad_io(
        void *state, struct chunk *chunk,
        enum io io, id src,
        const word *args, size_t len)
{
    struct im_nomad *nomad = state;

    switch(io)
    {
    case IO_PING: { chunk_io(chunk, IO_PONG, nomad->id, src, NULL, 0); return; }
    case IO_STATE: { im_nomad_io_state(nomad, chunk, src, args, len); return; }
    case IO_RESET: { im_nomad_reset(nomad, chunk); return; }

    case IO_MOD: { im_nomad_io_mod(nomad, chunk, args, len); return; }
    case IO_GET: { im_nomad_io_get(nomad, chunk, src, args, len); return; }
    case IO_SET: { im_nomad_io_set(nomad, chunk, args, len); return; }

    case IO_ID: { im_nomad_io_id(nomad, chunk, args, len); return; }
    case IO_PACK: { im_nomad_io_pack(nomad, chunk, args, len); return; }
    case IO_LOAD: { im_nomad_io_load(nomad, chunk, args, len); return; }
    case IO_UNLOAD: { im_nomad_io_unload(nomad, chunk, args, len); return; }

    case IO_LAUNCH: { im_nomad_io_launch(nomad, chunk, args, len); return; }

    default: { return; }
    }
}

static const word im_nomad_io_list[] =
{
    IO_PING,
    IO_STATE,
    IO_RESET,

    IO_MOD,
    IO_GET,
    IO_SET,

    IO_ID,
    IO_PACK,
    IO_LOAD,
    IO_UNLOAD,

    IO_LAUNCH,
};


// -----------------------------------------------------------------------------
// flow
// -----------------------------------------------------------------------------

static bool im_nomad_flow(const void *state, struct flow *flow)
{
    const struct im_nomad *nomad = state;
    if (!nomad->op) return false;

    *flow = (struct flow) {
        .id = nomad->id,
        .loops = nomad->loops,
        .target = nomad->item,
        .rank = tapes_info(nomad->item)->rank + 1,
    };

    switch (nomad->op)
    {
    case im_nomad_pack:
    case im_nomad_load: { flow->in = nomad->item; break; }
    case im_nomad_unload:{ flow->out = nomad->item; break; }
    case im_nomad_nil:
    default: { assert(false); }
    }

    return true;
}
