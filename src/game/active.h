/* active.h
   Rémi Attab (remi.attab@gmail.com), 06 Jun 2021
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

#include "common.h"
#include "game/item.h"
#include "game/prog.h"
#include "vm/vm.h"

struct chunk;


// -----------------------------------------------------------------------------
// item_config
// -----------------------------------------------------------------------------

typedef void (*init_fn_t) (void *state, id_t id, struct chunk *);
typedef void (*step_fn_t) (void *state, struct chunk *);
typedef void (*cmd_fn_t) (
        void *state, struct chunk *,
        enum atom_io cmd, id_t src, size_t len, const word_t *args);

struct item_config
{
    size_t size;
    init_fn_t init;
    step_fn_t step;
    cmd_fn_t cmd;
};

const struct item_config *item_config(item_t);

// -----------------------------------------------------------------------------
// progable
// -----------------------------------------------------------------------------

enum legion_packed progable_state
{
    progable_nil = 0,
    progable_blocked,
    progable_error,
};

struct legion_packed progable
{
    id_t id;
    uint16_t loops;
    enum progable_state state;

    prog_it_t index;
    const struct prog *prog;
};
static_assert(sizeof(struct progable) == 16);

static const uint16_t progable_loops_inf = UINT16_MAX;


// -----------------------------------------------------------------------------
// else
// -----------------------------------------------------------------------------
