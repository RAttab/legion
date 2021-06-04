/* deployer.c
   Rémi Attab (remi.attab@gmail.com), 04 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#include "common.h"
#include "game/atoms.h"
#include "game/item.h"
#include "game/chunk.h"
#include "game/config.h"

// -----------------------------------------------------------------------------
// deployer
// -----------------------------------------------------------------------------

static const uint16_t deployer_loop_inf = UINT16_MAX;

struct legion_packed deployer
{
    id_t id;
    uint16_t loops;
    uint8_t blocked; // bool would take 4 bits.
    prog_it_t index;
    const struct prog *prog;
};

static_assert(sizeof(struct deployer) == 16);

static void deployer_init(void *state, id_t id, struct chunk *chunk)
{
    struct deployer *deployer = state;
    (void) chunk;

    *deployer = (struct deployer) { .id = id };
}


// -----------------------------------------------------------------------------
// step
// -----------------------------------------------------------------------------

static void deployer_step_eof(struct deployer *deployer)
{
    if (deployer->loops != deployer_loop_inf) deployer->loops--;
    if (deployer->loops) deployer->index = 0;
}

static void deployer_step_input(struct deployer *deployer, struct chunk *chunk, item_t item)
{
    if (!deployer->blocked) {
        chunk_io_request(chunk, deployer->id, item);
        deployer->blocked = true;
        return;
    }

    item_t consumed = chunk_io_consume(chunk, deployer->id);
    if (consumed == ITEM_NIL) return;

    assert(consumed == item);
    deployer->blocked = false;
    deployer->index++;
}

static void deployer_step_output(struct deployer *deployer, struct chunk *chunk, item_t item)
{
    chunk_create(chunk, item);
    deployer->index++;
}

static void deployer_step(void *state, struct chunk *chunk)
{
    struct deployer *deployer = state;
    if (!deployer->prog) return;

    struct prog_ret ret = prog_at(deployer->prog, deployer->index);
    switch (ret.state) {
    case prog_eof: { deployer_step_eof(deployer); return; }
    case prog_input: { deployer_step_input(deployer, chunk, ret.item); return; }
    case prog_output: { deployer_step_output(deployer, chunk, ret.item); return; }
    default: { assert(false); }
    }
}


// -----------------------------------------------------------------------------
// cmd
// -----------------------------------------------------------------------------

static void deployer_cmd_reset(struct deployer *deployer, struct chunk *chunk)
{
    chunk_io_reset(chunk, deployer->id);
    *deployer = (struct deployer) {
        .id = deployer->id,
        .blocked = false,
        .loops = 0,
        .prog = NULL,
        .index = 0,
    };
}

static void deployer_cmd_prog(
        struct deployer *deployer, struct chunk *chunk, size_t len, const word_t *args)
{
    if (len < 1) return;

    uint32_t id, loops;
    vm_unpack(args[0], &id, &loops);

    if (!loops) loops = deployer_loop_inf;
    if (id != (prog_id_t) id) return;

    const struct prog *prog = prog_fetch(id);
    if (!prog || prog_host(prog) != ITEM_DEPLOYER) return;

    deployer_cmd_reset(deployer, chunk);
    deployer->loops = loops;
    deployer->prog = prog;
}

static void deployer_cmd(
        void *state, struct chunk *chunk,
        enum atom_io cmd, id_t src, size_t len, const word_t *args)
{
    struct deployer *deployer = state;
    (void) src;

    if (cmd == IO_PING) { chunk_cmd(chunk, IO_PONG, deployer->id, src, 0, NULL); return; }
    if (cmd == IO_PROG) { deployer_cmd_prog(deployer, chunk, len, args); return; }
    if (cmd == IO_RESET) { deployer_cmd_reset(deployer, chunk); return; }
    return;
}


// -----------------------------------------------------------------------------
// config
// -----------------------------------------------------------------------------

const struct item_config *deployer_config(void)
{
    static const struct item_config config = {
        .size = sizeof(struct deployer),
        .init = deployer_init,
        .step = deployer_step,
        .cmd = deployer_cmd,
    };
    return &config;
}
