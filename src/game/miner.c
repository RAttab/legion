/* miner.c
   Rémi Attab (remi.attab@gmail.com), 03 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/atoms.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/config.h"

// -----------------------------------------------------------------------------
// miner
// -----------------------------------------------------------------------------

static const uint16_t miner_loop_inf = UINT16_MAX;

struct legion_packed miner
{
    id_t id;
    uint16_t loops;
    uint8_t blocked; // bool would take 4 bits.
    prog_it_t index;
    const struct prog *prog;
};

static_assert(sizeof(struct miner) == 16);

static void miner_init(void *state, id_t id, struct chunk *chunk)
{
    struct miner *miner = state;
    (void) chunk;

    *miner = (struct miner) { .id = id };
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void miner_step_eof(struct miner *miner)
{
    if (miner->loops != miner_loop_inf) miner->loops--;
    if (miner->loops) miner->index = 0;
}

static void miner_step_input(struct miner *miner, struct chunk *chunk, item_t item)
{
    if (!miner->blocked) {
        chunk_io_request(chunk, miner->id, item);
        miner->blocked = true;
        return;
    }

    item_t consumed = chunk_io_consume(chunk, miner->id);
    if (consumed == ITEM_NIL) return;

    assert(consumed == item);
    miner->blocked = false;
    miner->index++;
}

static void miner_step_output(struct miner *miner, struct chunk *chunk, item_t item)
{
    if (!miner->blocked) {
        if (!chunk_harvest(chunk, item)) return;
    }

    miner->blocked = !chunk_io_produce(chunk, miner->id, item);
    if (!miner->blocked) miner->index++;
}

static void miner_step(void *state, struct chunk *chunk)
{
    struct miner *miner = state;
    if (!miner->prog) return;

    struct prog_ret ret = prog_at(miner->prog, miner->index);
    switch (ret.state) {
    case prog_eof: { miner_step_eof(miner); return; }
    case prog_input: { miner_step_input(miner, chunk, ret.item); return; }
    case prog_output: { miner_step_output(miner, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

static void miner_cmd_reset(struct miner *miner, struct chunk *chunk)
{
    chunk_io_reset(chunk, miner->id);
    *miner = (struct miner) {
        .id = miner->id,
        .blocked = false,
        .loops = 0,
        .prog = NULL,
        .index = 0,
    };
}

static void miner_cmd_prog(
        struct miner *miner, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    uint32_t id, loops;
    vm_unpack(args[0], &id, &loops);

    if (!loops) loops = miner_loop_inf;
    if (id != (prog_id_t) id) return;

    const struct prog *prog = prog_fetch(id);
    if (!prog || prog_host(prog) != ITEM_MINER) return;

    miner_cmd_reset(miner, chunk);
    miner->loops = loops;
    miner->prog = prog;
}

static void miner_cmd(
        void *state, struct chunk *chunk,
        enum atom_io cmd, id_t src, size_t len, const word_t *args)
{
    struct miner *miner = state;
    (void) src;

    if (cmd == IO_PING) { chunk_cmd(chunk, IO_PONG, miner->id, src, 0, NULL); return; }
    if (cmd == IO_PROG) { miner_cmd_prog(miner, chunk, len, args); return; }
    if (cmd == IO_RESET) { miner_cmd_reset(miner, chunk); return; }
    return;
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct item_config *miner_config(void)
{
    static const struct item_config config = {
        .size = sizeof(struct miner),
        .init = miner_init,
        .step = miner_step,
        .cmd = miner_cmd,
    };
    return &config;
}
