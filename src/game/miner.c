/* miner.c
   Rémi Attab (remi.attab@gmail.com), 03 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "game/atoms.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/config.h"

// -----------------------------------------------------------------------------
// miner
// -----------------------------------------------------------------------------

struct miner
{
    id_t id;
    bool blocked;
    uint32_t loops;

    const struct prog *prog;
    struct prog_it it;
};

static void miner_init(void *state, id_t id, struct chunk *chunk)
{
    (void) id; (void) chunk;
    struct miner *miner = state;
    *miner = (struct miner) { .id = id, 0 };
}

static void miner_step_eof(struct miner *miner)
{
    if (miner->loops != UINT32_MAX) miner->loops--;
    if (miner->loops) miner->it = prog_begin(miner->prog);
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
    prog_next(&miner->it);
}

static void miner_step_output(struct miner *miner, struct chunk *chunk, item_t item)
{
    if (!miner->blocked) {
        if (!chunk_harvest(chunk, item)) return;
    }

    miner->blocked = !chunk_io_produce(chunk, miner->id, item);
    if (!miner->blocked) prog_next(&miner->it);
}

static void miner_step(void *state, struct chunk *chunk)
{
    struct miner *miner = state;
    if (!miner->prog) return;

    struct prog_ret ret = prog_peek(&miner->it);
    switch (ret.state) {
    case prog_eof: { miner_step_eof(miner); return; }
    case prog_input: { miner_step_input(miner, chunk, ret.item); return; }
    case prog_output: { miner_step_output(miner, chunk, ret.item); return; }
    default: { assert(false); }
    }
}

static void miner_reset(struct miner *miner, struct chunk *chunk)
{
    chunk_io_reset(chunk, miner->id);
    miner->blocked = false;
    miner->loops = 0;
    miner->prog = NULL;
    miner->it = (struct prog_it) {0};
}

static void miner_prog(struct miner *miner, struct chunk *chunk, word_t arg)
{
    uint32_t id, loops;
    vm_unpack(arg, &id, &loops);

    if (!loops) loops = UINT32_MAX;
    if (id != (prog_id_t) id) return;

    const struct prog *prog = prog_fetch(id);
    if (!prog) return;

    miner_reset(miner, chunk);
    miner->loops = loops;
    miner->prog = prog;
    miner->it = prog_begin(prog);
}

static void miner_cmd(
        void *state, struct chunk *chunk, id_t src, enum atom_io cmd, word_t arg)
{
    (void) src;
    struct miner *miner = state;

    switch (cmd) {
    case IO_PROG: { miner_prog(miner, chunk, arg); return; }
    case IO_RESET: { miner_reset(miner, chunk); return; }
    default: { return; }
    }
}

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
